/* Automatically generated from Vulkan vk.xml; DO NOT EDIT!
 *
 * This file is generated from Vulkan vk.xml file covered
 * by the following copyright and permission notice:
 *
 * Copyright 2015-2024 The Khronos Group Inc.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __WINE_VULKAN_THUNKS_H
#define __WINE_VULKAN_THUNKS_H

#define WINE_VK_VERSION VK_API_VERSION_1_3

/* Functions for which we have custom implementations outside of the thunks. */
VkResult wine_vkAllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo *pAllocateInfo, VkCommandBuffer *pCommandBuffers);
VkResult wine_vkAllocateMemory(VkDevice device, const VkMemoryAllocateInfo *pAllocateInfo, const VkAllocationCallbacks *pAllocator, VkDeviceMemory *pMemory);
VkResult wine_vkCreateBuffer(VkDevice device, const VkBufferCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkBuffer *pBuffer);
VkResult wine_vkCreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkCommandPool *pCommandPool, void *client_ptr);
VkResult wine_vkCreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugReportCallbackEXT *pCallback);
VkResult wine_vkCreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pMessenger);
VkResult wine_vkCreateDeferredOperationKHR(VkDevice device, const VkAllocationCallbacks *pAllocator, VkDeferredOperationKHR *pDeferredOperation);
VkResult wine_vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDevice *pDevice, void *client_ptr);
VkResult wine_vkCreateImage(VkDevice device, const VkImageCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkImage *pImage);
VkResult wine_vkCreateInstance(const VkInstanceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkInstance *pInstance, void *client_ptr);
VkResult wine_vkCreateWin32SurfaceKHR(VkInstance instance, const VkWin32SurfaceCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface);
void wine_vkDestroyCommandPool(VkDevice device, VkCommandPool commandPool, const VkAllocationCallbacks *pAllocator);
void wine_vkDestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks *pAllocator);
void wine_vkDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks *pAllocator);
void wine_vkDestroyDeferredOperationKHR(VkDevice device, VkDeferredOperationKHR operation, const VkAllocationCallbacks *pAllocator);
void wine_vkDestroyDevice(VkDevice device, const VkAllocationCallbacks *pAllocator);
void wine_vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks *pAllocator);
void wine_vkDestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks *pAllocator);
VkResult wine_vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char *pLayerName, uint32_t *pPropertyCount, VkExtensionProperties *pProperties);
VkResult wine_vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount, VkLayerProperties *pProperties);
VkResult wine_vkEnumerateInstanceExtensionProperties(const char *pLayerName, uint32_t *pPropertyCount, VkExtensionProperties *pProperties);
VkResult wine_vkEnumerateInstanceVersion(uint32_t *pApiVersion);
VkResult wine_vkEnumeratePhysicalDeviceGroups(VkInstance instance, uint32_t *pPhysicalDeviceGroupCount, VkPhysicalDeviceGroupProperties *pPhysicalDeviceGroupProperties);
VkResult wine_vkEnumeratePhysicalDeviceGroupsKHR(VkInstance instance, uint32_t *pPhysicalDeviceGroupCount, VkPhysicalDeviceGroupProperties *pPhysicalDeviceGroupProperties);
VkResult wine_vkEnumeratePhysicalDevices(VkInstance instance, uint32_t *pPhysicalDeviceCount, VkPhysicalDevice *pPhysicalDevices);
void wine_vkFreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount, const VkCommandBuffer *pCommandBuffers);
void wine_vkFreeMemory(VkDevice device, VkDeviceMemory memory, const VkAllocationCallbacks *pAllocator);
VkResult wine_vkGetCalibratedTimestampsEXT(VkDevice device, uint32_t timestampCount, const VkCalibratedTimestampInfoKHR *pTimestampInfos, uint64_t *pTimestamps, uint64_t *pMaxDeviation);
VkResult wine_vkGetCalibratedTimestampsKHR(VkDevice device, uint32_t timestampCount, const VkCalibratedTimestampInfoKHR *pTimestampInfos, uint64_t *pTimestamps, uint64_t *pMaxDeviation);
void wine_vkGetDeviceQueue(VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue *pQueue);
void wine_vkGetDeviceQueue2(VkDevice device, const VkDeviceQueueInfo2 *pQueueInfo, VkQueue *pQueue);
VkResult wine_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT(VkPhysicalDevice physicalDevice, uint32_t *pTimeDomainCount, VkTimeDomainKHR *pTimeDomains);
VkResult wine_vkGetPhysicalDeviceCalibrateableTimeDomainsKHR(VkPhysicalDevice physicalDevice, uint32_t *pTimeDomainCount, VkTimeDomainKHR *pTimeDomains);
void wine_vkGetPhysicalDeviceExternalBufferProperties(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalBufferInfo *pExternalBufferInfo, VkExternalBufferProperties *pExternalBufferProperties);
void wine_vkGetPhysicalDeviceExternalBufferPropertiesKHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalBufferInfo *pExternalBufferInfo, VkExternalBufferProperties *pExternalBufferProperties);
void wine_vkGetPhysicalDeviceExternalFenceProperties(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalFenceInfo *pExternalFenceInfo, VkExternalFenceProperties *pExternalFenceProperties);
void wine_vkGetPhysicalDeviceExternalFencePropertiesKHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalFenceInfo *pExternalFenceInfo, VkExternalFenceProperties *pExternalFenceProperties);
void wine_vkGetPhysicalDeviceExternalSemaphoreProperties(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalSemaphoreInfo *pExternalSemaphoreInfo, VkExternalSemaphoreProperties *pExternalSemaphoreProperties);
void wine_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalSemaphoreInfo *pExternalSemaphoreInfo, VkExternalSemaphoreProperties *pExternalSemaphoreProperties);
VkResult wine_vkGetPhysicalDeviceImageFormatProperties2(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceImageFormatInfo2 *pImageFormatInfo, VkImageFormatProperties2 *pImageFormatProperties);
VkResult wine_vkGetPhysicalDeviceImageFormatProperties2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceImageFormatInfo2 *pImageFormatInfo, VkImageFormatProperties2 *pImageFormatProperties);
VkResult wine_vkGetPhysicalDeviceSurfaceCapabilities2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR *pSurfaceInfo, VkSurfaceCapabilities2KHR *pSurfaceCapabilities);
VkResult wine_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR *pSurfaceCapabilities);
VkResult wine_vkMapMemory(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void **ppData);
VkResult wine_vkMapMemory2KHR(VkDevice device, const VkMemoryMapInfoKHR *pMemoryMapInfo, void **ppData);
void wine_vkUnmapMemory(VkDevice device, VkDeviceMemory memory);
VkResult wine_vkUnmapMemory2KHR(VkDevice device, const VkMemoryUnmapInfoKHR *pMemoryUnmapInfo);

/* For use by vkDevice and children */
struct vulkan_device_funcs
{
    VkResult (*p_vkAcquireNextImage2KHR)(VkDevice, const VkAcquireNextImageInfoKHR *, uint32_t *);
    VkResult (*p_vkAcquireNextImageKHR)(VkDevice, VkSwapchainKHR, uint64_t, VkSemaphore, VkFence, uint32_t *);
    VkResult (*p_vkAcquirePerformanceConfigurationINTEL)(VkDevice, const VkPerformanceConfigurationAcquireInfoINTEL *, VkPerformanceConfigurationINTEL *);
    VkResult (*p_vkAcquireProfilingLockKHR)(VkDevice, const VkAcquireProfilingLockInfoKHR *);
    VkResult (*p_vkAllocateCommandBuffers)(VkDevice, const VkCommandBufferAllocateInfo *, VkCommandBuffer *);
    VkResult (*p_vkAllocateDescriptorSets)(VkDevice, const VkDescriptorSetAllocateInfo *, VkDescriptorSet *);
    VkResult (*p_vkAllocateMemory)(VkDevice, const VkMemoryAllocateInfo *, const VkAllocationCallbacks *, VkDeviceMemory *);
    VkResult (*p_vkBeginCommandBuffer)(VkCommandBuffer, const VkCommandBufferBeginInfo *);
    VkResult (*p_vkBindAccelerationStructureMemoryNV)(VkDevice, uint32_t, const VkBindAccelerationStructureMemoryInfoNV *);
    VkResult (*p_vkBindBufferMemory)(VkDevice, VkBuffer, VkDeviceMemory, VkDeviceSize);
    VkResult (*p_vkBindBufferMemory2)(VkDevice, uint32_t, const VkBindBufferMemoryInfo *);
    VkResult (*p_vkBindBufferMemory2KHR)(VkDevice, uint32_t, const VkBindBufferMemoryInfo *);
    VkResult (*p_vkBindImageMemory)(VkDevice, VkImage, VkDeviceMemory, VkDeviceSize);
    VkResult (*p_vkBindImageMemory2)(VkDevice, uint32_t, const VkBindImageMemoryInfo *);
    VkResult (*p_vkBindImageMemory2KHR)(VkDevice, uint32_t, const VkBindImageMemoryInfo *);
    VkResult (*p_vkBindOpticalFlowSessionImageNV)(VkDevice, VkOpticalFlowSessionNV, VkOpticalFlowSessionBindingPointNV, VkImageView, VkImageLayout);
    VkResult (*p_vkBuildAccelerationStructuresKHR)(VkDevice, VkDeferredOperationKHR, uint32_t, const VkAccelerationStructureBuildGeometryInfoKHR *, const VkAccelerationStructureBuildRangeInfoKHR * const*);
    VkResult (*p_vkBuildMicromapsEXT)(VkDevice, VkDeferredOperationKHR, uint32_t, const VkMicromapBuildInfoEXT *);
    void (*p_vkCmdBeginConditionalRenderingEXT)(VkCommandBuffer, const VkConditionalRenderingBeginInfoEXT *);
    void (*p_vkCmdBeginDebugUtilsLabelEXT)(VkCommandBuffer, const VkDebugUtilsLabelEXT *);
    void (*p_vkCmdBeginQuery)(VkCommandBuffer, VkQueryPool, uint32_t, VkQueryControlFlags);
    void (*p_vkCmdBeginQueryIndexedEXT)(VkCommandBuffer, VkQueryPool, uint32_t, VkQueryControlFlags, uint32_t);
    void (*p_vkCmdBeginRenderPass)(VkCommandBuffer, const VkRenderPassBeginInfo *, VkSubpassContents);
    void (*p_vkCmdBeginRenderPass2)(VkCommandBuffer, const VkRenderPassBeginInfo *, const VkSubpassBeginInfo *);
    void (*p_vkCmdBeginRenderPass2KHR)(VkCommandBuffer, const VkRenderPassBeginInfo *, const VkSubpassBeginInfo *);
    void (*p_vkCmdBeginRendering)(VkCommandBuffer, const VkRenderingInfo *);
    void (*p_vkCmdBeginRenderingKHR)(VkCommandBuffer, const VkRenderingInfo *);
    void (*p_vkCmdBeginTransformFeedbackEXT)(VkCommandBuffer, uint32_t, uint32_t, const VkBuffer *, const VkDeviceSize *);
    void (*p_vkCmdBindDescriptorBufferEmbeddedSamplers2EXT)(VkCommandBuffer, const VkBindDescriptorBufferEmbeddedSamplersInfoEXT *);
    void (*p_vkCmdBindDescriptorBufferEmbeddedSamplersEXT)(VkCommandBuffer, VkPipelineBindPoint, VkPipelineLayout, uint32_t);
    void (*p_vkCmdBindDescriptorBuffersEXT)(VkCommandBuffer, uint32_t, const VkDescriptorBufferBindingInfoEXT *);
    void (*p_vkCmdBindDescriptorSets)(VkCommandBuffer, VkPipelineBindPoint, VkPipelineLayout, uint32_t, uint32_t, const VkDescriptorSet *, uint32_t, const uint32_t *);
    void (*p_vkCmdBindDescriptorSets2KHR)(VkCommandBuffer, const VkBindDescriptorSetsInfoKHR *);
    void (*p_vkCmdBindIndexBuffer)(VkCommandBuffer, VkBuffer, VkDeviceSize, VkIndexType);
    void (*p_vkCmdBindIndexBuffer2KHR)(VkCommandBuffer, VkBuffer, VkDeviceSize, VkDeviceSize, VkIndexType);
    void (*p_vkCmdBindInvocationMaskHUAWEI)(VkCommandBuffer, VkImageView, VkImageLayout);
    void (*p_vkCmdBindPipeline)(VkCommandBuffer, VkPipelineBindPoint, VkPipeline);
    void (*p_vkCmdBindPipelineShaderGroupNV)(VkCommandBuffer, VkPipelineBindPoint, VkPipeline, uint32_t);
    void (*p_vkCmdBindShadersEXT)(VkCommandBuffer, uint32_t, const VkShaderStageFlagBits *, const VkShaderEXT *);
    void (*p_vkCmdBindShadingRateImageNV)(VkCommandBuffer, VkImageView, VkImageLayout);
    void (*p_vkCmdBindTransformFeedbackBuffersEXT)(VkCommandBuffer, uint32_t, uint32_t, const VkBuffer *, const VkDeviceSize *, const VkDeviceSize *);
    void (*p_vkCmdBindVertexBuffers)(VkCommandBuffer, uint32_t, uint32_t, const VkBuffer *, const VkDeviceSize *);
    void (*p_vkCmdBindVertexBuffers2)(VkCommandBuffer, uint32_t, uint32_t, const VkBuffer *, const VkDeviceSize *, const VkDeviceSize *, const VkDeviceSize *);
    void (*p_vkCmdBindVertexBuffers2EXT)(VkCommandBuffer, uint32_t, uint32_t, const VkBuffer *, const VkDeviceSize *, const VkDeviceSize *, const VkDeviceSize *);
    void (*p_vkCmdBlitImage)(VkCommandBuffer, VkImage, VkImageLayout, VkImage, VkImageLayout, uint32_t, const VkImageBlit *, VkFilter);
    void (*p_vkCmdBlitImage2)(VkCommandBuffer, const VkBlitImageInfo2 *);
    void (*p_vkCmdBlitImage2KHR)(VkCommandBuffer, const VkBlitImageInfo2 *);
    void (*p_vkCmdBuildAccelerationStructureNV)(VkCommandBuffer, const VkAccelerationStructureInfoNV *, VkBuffer, VkDeviceSize, VkBool32, VkAccelerationStructureNV, VkAccelerationStructureNV, VkBuffer, VkDeviceSize);
    void (*p_vkCmdBuildAccelerationStructuresIndirectKHR)(VkCommandBuffer, uint32_t, const VkAccelerationStructureBuildGeometryInfoKHR *, const VkDeviceAddress *, const uint32_t *, const uint32_t * const*);
    void (*p_vkCmdBuildAccelerationStructuresKHR)(VkCommandBuffer, uint32_t, const VkAccelerationStructureBuildGeometryInfoKHR *, const VkAccelerationStructureBuildRangeInfoKHR * const*);
    void (*p_vkCmdBuildMicromapsEXT)(VkCommandBuffer, uint32_t, const VkMicromapBuildInfoEXT *);
    void (*p_vkCmdClearAttachments)(VkCommandBuffer, uint32_t, const VkClearAttachment *, uint32_t, const VkClearRect *);
    void (*p_vkCmdClearColorImage)(VkCommandBuffer, VkImage, VkImageLayout, const VkClearColorValue *, uint32_t, const VkImageSubresourceRange *);
    void (*p_vkCmdClearDepthStencilImage)(VkCommandBuffer, VkImage, VkImageLayout, const VkClearDepthStencilValue *, uint32_t, const VkImageSubresourceRange *);
    void (*p_vkCmdCopyAccelerationStructureKHR)(VkCommandBuffer, const VkCopyAccelerationStructureInfoKHR *);
    void (*p_vkCmdCopyAccelerationStructureNV)(VkCommandBuffer, VkAccelerationStructureNV, VkAccelerationStructureNV, VkCopyAccelerationStructureModeKHR);
    void (*p_vkCmdCopyAccelerationStructureToMemoryKHR)(VkCommandBuffer, const VkCopyAccelerationStructureToMemoryInfoKHR *);
    void (*p_vkCmdCopyBuffer)(VkCommandBuffer, VkBuffer, VkBuffer, uint32_t, const VkBufferCopy *);
    void (*p_vkCmdCopyBuffer2)(VkCommandBuffer, const VkCopyBufferInfo2 *);
    void (*p_vkCmdCopyBuffer2KHR)(VkCommandBuffer, const VkCopyBufferInfo2 *);
    void (*p_vkCmdCopyBufferToImage)(VkCommandBuffer, VkBuffer, VkImage, VkImageLayout, uint32_t, const VkBufferImageCopy *);
    void (*p_vkCmdCopyBufferToImage2)(VkCommandBuffer, const VkCopyBufferToImageInfo2 *);
    void (*p_vkCmdCopyBufferToImage2KHR)(VkCommandBuffer, const VkCopyBufferToImageInfo2 *);
    void (*p_vkCmdCopyImage)(VkCommandBuffer, VkImage, VkImageLayout, VkImage, VkImageLayout, uint32_t, const VkImageCopy *);
    void (*p_vkCmdCopyImage2)(VkCommandBuffer, const VkCopyImageInfo2 *);
    void (*p_vkCmdCopyImage2KHR)(VkCommandBuffer, const VkCopyImageInfo2 *);
    void (*p_vkCmdCopyImageToBuffer)(VkCommandBuffer, VkImage, VkImageLayout, VkBuffer, uint32_t, const VkBufferImageCopy *);
    void (*p_vkCmdCopyImageToBuffer2)(VkCommandBuffer, const VkCopyImageToBufferInfo2 *);
    void (*p_vkCmdCopyImageToBuffer2KHR)(VkCommandBuffer, const VkCopyImageToBufferInfo2 *);
    void (*p_vkCmdCopyMemoryIndirectNV)(VkCommandBuffer, VkDeviceAddress, uint32_t, uint32_t);
    void (*p_vkCmdCopyMemoryToAccelerationStructureKHR)(VkCommandBuffer, const VkCopyMemoryToAccelerationStructureInfoKHR *);
    void (*p_vkCmdCopyMemoryToImageIndirectNV)(VkCommandBuffer, VkDeviceAddress, uint32_t, uint32_t, VkImage, VkImageLayout, const VkImageSubresourceLayers *);
    void (*p_vkCmdCopyMemoryToMicromapEXT)(VkCommandBuffer, const VkCopyMemoryToMicromapInfoEXT *);
    void (*p_vkCmdCopyMicromapEXT)(VkCommandBuffer, const VkCopyMicromapInfoEXT *);
    void (*p_vkCmdCopyMicromapToMemoryEXT)(VkCommandBuffer, const VkCopyMicromapToMemoryInfoEXT *);
    void (*p_vkCmdCopyQueryPoolResults)(VkCommandBuffer, VkQueryPool, uint32_t, uint32_t, VkBuffer, VkDeviceSize, VkDeviceSize, VkQueryResultFlags);
    void (*p_vkCmdCuLaunchKernelNVX)(VkCommandBuffer, const VkCuLaunchInfoNVX *);
    void (*p_vkCmdCudaLaunchKernelNV)(VkCommandBuffer, const VkCudaLaunchInfoNV *);
    void (*p_vkCmdDebugMarkerBeginEXT)(VkCommandBuffer, const VkDebugMarkerMarkerInfoEXT *);
    void (*p_vkCmdDebugMarkerEndEXT)(VkCommandBuffer);
    void (*p_vkCmdDebugMarkerInsertEXT)(VkCommandBuffer, const VkDebugMarkerMarkerInfoEXT *);
    void (*p_vkCmdDecompressMemoryIndirectCountNV)(VkCommandBuffer, VkDeviceAddress, VkDeviceAddress, uint32_t);
    void (*p_vkCmdDecompressMemoryNV)(VkCommandBuffer, uint32_t, const VkDecompressMemoryRegionNV *);
    void (*p_vkCmdDispatch)(VkCommandBuffer, uint32_t, uint32_t, uint32_t);
    void (*p_vkCmdDispatchBase)(VkCommandBuffer, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
    void (*p_vkCmdDispatchBaseKHR)(VkCommandBuffer, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
    void (*p_vkCmdDispatchIndirect)(VkCommandBuffer, VkBuffer, VkDeviceSize);
    void (*p_vkCmdDraw)(VkCommandBuffer, uint32_t, uint32_t, uint32_t, uint32_t);
    void (*p_vkCmdDrawClusterHUAWEI)(VkCommandBuffer, uint32_t, uint32_t, uint32_t);
    void (*p_vkCmdDrawClusterIndirectHUAWEI)(VkCommandBuffer, VkBuffer, VkDeviceSize);
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
    void (*p_vkCmdExecuteGeneratedCommandsNV)(VkCommandBuffer, VkBool32, const VkGeneratedCommandsInfoNV *);
    void (*p_vkCmdFillBuffer)(VkCommandBuffer, VkBuffer, VkDeviceSize, VkDeviceSize, uint32_t);
    void (*p_vkCmdInsertDebugUtilsLabelEXT)(VkCommandBuffer, const VkDebugUtilsLabelEXT *);
    void (*p_vkCmdNextSubpass)(VkCommandBuffer, VkSubpassContents);
    void (*p_vkCmdNextSubpass2)(VkCommandBuffer, const VkSubpassBeginInfo *, const VkSubpassEndInfo *);
    void (*p_vkCmdNextSubpass2KHR)(VkCommandBuffer, const VkSubpassBeginInfo *, const VkSubpassEndInfo *);
    void (*p_vkCmdOpticalFlowExecuteNV)(VkCommandBuffer, VkOpticalFlowSessionNV, const VkOpticalFlowExecuteInfoNV *);
    void (*p_vkCmdPipelineBarrier)(VkCommandBuffer, VkPipelineStageFlags, VkPipelineStageFlags, VkDependencyFlags, uint32_t, const VkMemoryBarrier *, uint32_t, const VkBufferMemoryBarrier *, uint32_t, const VkImageMemoryBarrier *);
    void (*p_vkCmdPipelineBarrier2)(VkCommandBuffer, const VkDependencyInfo *);
    void (*p_vkCmdPipelineBarrier2KHR)(VkCommandBuffer, const VkDependencyInfo *);
    void (*p_vkCmdPreprocessGeneratedCommandsNV)(VkCommandBuffer, const VkGeneratedCommandsInfoNV *);
    void (*p_vkCmdPushConstants)(VkCommandBuffer, VkPipelineLayout, VkShaderStageFlags, uint32_t, uint32_t, const void *);
    void (*p_vkCmdPushConstants2KHR)(VkCommandBuffer, const VkPushConstantsInfoKHR *);
    void (*p_vkCmdPushDescriptorSet2KHR)(VkCommandBuffer, const VkPushDescriptorSetInfoKHR *);
    void (*p_vkCmdPushDescriptorSetKHR)(VkCommandBuffer, VkPipelineBindPoint, VkPipelineLayout, uint32_t, uint32_t, const VkWriteDescriptorSet *);
    void (*p_vkCmdPushDescriptorSetWithTemplate2KHR)(VkCommandBuffer, const VkPushDescriptorSetWithTemplateInfoKHR *);
    void (*p_vkCmdPushDescriptorSetWithTemplateKHR)(VkCommandBuffer, VkDescriptorUpdateTemplate, VkPipelineLayout, uint32_t, const void *);
    void (*p_vkCmdResetEvent)(VkCommandBuffer, VkEvent, VkPipelineStageFlags);
    void (*p_vkCmdResetEvent2)(VkCommandBuffer, VkEvent, VkPipelineStageFlags2);
    void (*p_vkCmdResetEvent2KHR)(VkCommandBuffer, VkEvent, VkPipelineStageFlags2);
    void (*p_vkCmdResetQueryPool)(VkCommandBuffer, VkQueryPool, uint32_t, uint32_t);
    void (*p_vkCmdResolveImage)(VkCommandBuffer, VkImage, VkImageLayout, VkImage, VkImageLayout, uint32_t, const VkImageResolve *);
    void (*p_vkCmdResolveImage2)(VkCommandBuffer, const VkResolveImageInfo2 *);
    void (*p_vkCmdResolveImage2KHR)(VkCommandBuffer, const VkResolveImageInfo2 *);
    void (*p_vkCmdSetAlphaToCoverageEnableEXT)(VkCommandBuffer, VkBool32);
    void (*p_vkCmdSetAlphaToOneEnableEXT)(VkCommandBuffer, VkBool32);
    void (*p_vkCmdSetAttachmentFeedbackLoopEnableEXT)(VkCommandBuffer, VkImageAspectFlags);
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
    void (*p_vkCmdSetDepthBias2EXT)(VkCommandBuffer, const VkDepthBiasInfoEXT *);
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
    void (*p_vkCmdSetDescriptorBufferOffsets2EXT)(VkCommandBuffer, const VkSetDescriptorBufferOffsetsInfoEXT *);
    void (*p_vkCmdSetDescriptorBufferOffsetsEXT)(VkCommandBuffer, VkPipelineBindPoint, VkPipelineLayout, uint32_t, uint32_t, const uint32_t *, const VkDeviceSize *);
    void (*p_vkCmdSetDeviceMask)(VkCommandBuffer, uint32_t);
    void (*p_vkCmdSetDeviceMaskKHR)(VkCommandBuffer, uint32_t);
    void (*p_vkCmdSetDiscardRectangleEXT)(VkCommandBuffer, uint32_t, uint32_t, const VkRect2D *);
    void (*p_vkCmdSetDiscardRectangleEnableEXT)(VkCommandBuffer, VkBool32);
    void (*p_vkCmdSetDiscardRectangleModeEXT)(VkCommandBuffer, VkDiscardRectangleModeEXT);
    void (*p_vkCmdSetEvent)(VkCommandBuffer, VkEvent, VkPipelineStageFlags);
    void (*p_vkCmdSetEvent2)(VkCommandBuffer, VkEvent, const VkDependencyInfo *);
    void (*p_vkCmdSetEvent2KHR)(VkCommandBuffer, VkEvent, const VkDependencyInfo *);
    void (*p_vkCmdSetExclusiveScissorEnableNV)(VkCommandBuffer, uint32_t, uint32_t, const VkBool32 *);
    void (*p_vkCmdSetExclusiveScissorNV)(VkCommandBuffer, uint32_t, uint32_t, const VkRect2D *);
    void (*p_vkCmdSetExtraPrimitiveOverestimationSizeEXT)(VkCommandBuffer, float);
    void (*p_vkCmdSetFragmentShadingRateEnumNV)(VkCommandBuffer, VkFragmentShadingRateNV, const VkFragmentShadingRateCombinerOpKHR[2]);
    void (*p_vkCmdSetFragmentShadingRateKHR)(VkCommandBuffer, const VkExtent2D *, const VkFragmentShadingRateCombinerOpKHR[2]);
    void (*p_vkCmdSetFrontFace)(VkCommandBuffer, VkFrontFace);
    void (*p_vkCmdSetFrontFaceEXT)(VkCommandBuffer, VkFrontFace);
    void (*p_vkCmdSetLineRasterizationModeEXT)(VkCommandBuffer, VkLineRasterizationModeEXT);
    void (*p_vkCmdSetLineStippleEXT)(VkCommandBuffer, uint32_t, uint16_t);
    void (*p_vkCmdSetLineStippleEnableEXT)(VkCommandBuffer, VkBool32);
    void (*p_vkCmdSetLineStippleKHR)(VkCommandBuffer, uint32_t, uint16_t);
    void (*p_vkCmdSetLineWidth)(VkCommandBuffer, float);
    void (*p_vkCmdSetLogicOpEXT)(VkCommandBuffer, VkLogicOp);
    void (*p_vkCmdSetLogicOpEnableEXT)(VkCommandBuffer, VkBool32);
    void (*p_vkCmdSetPatchControlPointsEXT)(VkCommandBuffer, uint32_t);
    VkResult (*p_vkCmdSetPerformanceMarkerINTEL)(VkCommandBuffer, const VkPerformanceMarkerInfoINTEL *);
    VkResult (*p_vkCmdSetPerformanceOverrideINTEL)(VkCommandBuffer, const VkPerformanceOverrideInfoINTEL *);
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
    void (*p_vkCmdSetRenderingAttachmentLocationsKHR)(VkCommandBuffer, const VkRenderingAttachmentLocationInfoKHR *);
    void (*p_vkCmdSetRenderingInputAttachmentIndicesKHR)(VkCommandBuffer, const VkRenderingInputAttachmentIndexInfoKHR *);
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
    void (*p_vkCmdTraceRaysIndirectKHR)(VkCommandBuffer, const VkStridedDeviceAddressRegionKHR *, const VkStridedDeviceAddressRegionKHR *, const VkStridedDeviceAddressRegionKHR *, const VkStridedDeviceAddressRegionKHR *, VkDeviceAddress);
    void (*p_vkCmdTraceRaysKHR)(VkCommandBuffer, const VkStridedDeviceAddressRegionKHR *, const VkStridedDeviceAddressRegionKHR *, const VkStridedDeviceAddressRegionKHR *, const VkStridedDeviceAddressRegionKHR *, uint32_t, uint32_t, uint32_t);
    void (*p_vkCmdTraceRaysNV)(VkCommandBuffer, VkBuffer, VkDeviceSize, VkBuffer, VkDeviceSize, VkDeviceSize, VkBuffer, VkDeviceSize, VkDeviceSize, VkBuffer, VkDeviceSize, VkDeviceSize, uint32_t, uint32_t, uint32_t);
    void (*p_vkCmdUpdateBuffer)(VkCommandBuffer, VkBuffer, VkDeviceSize, VkDeviceSize, const void *);
    void (*p_vkCmdUpdatePipelineIndirectBufferNV)(VkCommandBuffer, VkPipelineBindPoint, VkPipeline);
    void (*p_vkCmdWaitEvents)(VkCommandBuffer, uint32_t, const VkEvent *, VkPipelineStageFlags, VkPipelineStageFlags, uint32_t, const VkMemoryBarrier *, uint32_t, const VkBufferMemoryBarrier *, uint32_t, const VkImageMemoryBarrier *);
    void (*p_vkCmdWaitEvents2)(VkCommandBuffer, uint32_t, const VkEvent *, const VkDependencyInfo *);
    void (*p_vkCmdWaitEvents2KHR)(VkCommandBuffer, uint32_t, const VkEvent *, const VkDependencyInfo *);
    void (*p_vkCmdWriteAccelerationStructuresPropertiesKHR)(VkCommandBuffer, uint32_t, const VkAccelerationStructureKHR *, VkQueryType, VkQueryPool, uint32_t);
    void (*p_vkCmdWriteAccelerationStructuresPropertiesNV)(VkCommandBuffer, uint32_t, const VkAccelerationStructureNV *, VkQueryType, VkQueryPool, uint32_t);
    void (*p_vkCmdWriteBufferMarker2AMD)(VkCommandBuffer, VkPipelineStageFlags2, VkBuffer, VkDeviceSize, uint32_t);
    void (*p_vkCmdWriteBufferMarkerAMD)(VkCommandBuffer, VkPipelineStageFlagBits, VkBuffer, VkDeviceSize, uint32_t);
    void (*p_vkCmdWriteMicromapsPropertiesEXT)(VkCommandBuffer, uint32_t, const VkMicromapEXT *, VkQueryType, VkQueryPool, uint32_t);
    void (*p_vkCmdWriteTimestamp)(VkCommandBuffer, VkPipelineStageFlagBits, VkQueryPool, uint32_t);
    void (*p_vkCmdWriteTimestamp2)(VkCommandBuffer, VkPipelineStageFlags2, VkQueryPool, uint32_t);
    void (*p_vkCmdWriteTimestamp2KHR)(VkCommandBuffer, VkPipelineStageFlags2, VkQueryPool, uint32_t);
    VkResult (*p_vkCompileDeferredNV)(VkDevice, VkPipeline, uint32_t);
    VkResult (*p_vkCopyAccelerationStructureKHR)(VkDevice, VkDeferredOperationKHR, const VkCopyAccelerationStructureInfoKHR *);
    VkResult (*p_vkCopyAccelerationStructureToMemoryKHR)(VkDevice, VkDeferredOperationKHR, const VkCopyAccelerationStructureToMemoryInfoKHR *);
    VkResult (*p_vkCopyImageToImageEXT)(VkDevice, const VkCopyImageToImageInfoEXT *);
    VkResult (*p_vkCopyImageToMemoryEXT)(VkDevice, const VkCopyImageToMemoryInfoEXT *);
    VkResult (*p_vkCopyMemoryToAccelerationStructureKHR)(VkDevice, VkDeferredOperationKHR, const VkCopyMemoryToAccelerationStructureInfoKHR *);
    VkResult (*p_vkCopyMemoryToImageEXT)(VkDevice, const VkCopyMemoryToImageInfoEXT *);
    VkResult (*p_vkCopyMemoryToMicromapEXT)(VkDevice, VkDeferredOperationKHR, const VkCopyMemoryToMicromapInfoEXT *);
    VkResult (*p_vkCopyMicromapEXT)(VkDevice, VkDeferredOperationKHR, const VkCopyMicromapInfoEXT *);
    VkResult (*p_vkCopyMicromapToMemoryEXT)(VkDevice, VkDeferredOperationKHR, const VkCopyMicromapToMemoryInfoEXT *);
    VkResult (*p_vkCreateAccelerationStructureKHR)(VkDevice, const VkAccelerationStructureCreateInfoKHR *, const VkAllocationCallbacks *, VkAccelerationStructureKHR *);
    VkResult (*p_vkCreateAccelerationStructureNV)(VkDevice, const VkAccelerationStructureCreateInfoNV *, const VkAllocationCallbacks *, VkAccelerationStructureNV *);
    VkResult (*p_vkCreateBuffer)(VkDevice, const VkBufferCreateInfo *, const VkAllocationCallbacks *, VkBuffer *);
    VkResult (*p_vkCreateBufferView)(VkDevice, const VkBufferViewCreateInfo *, const VkAllocationCallbacks *, VkBufferView *);
    VkResult (*p_vkCreateCommandPool)(VkDevice, const VkCommandPoolCreateInfo *, const VkAllocationCallbacks *, VkCommandPool *);
    VkResult (*p_vkCreateComputePipelines)(VkDevice, VkPipelineCache, uint32_t, const VkComputePipelineCreateInfo *, const VkAllocationCallbacks *, VkPipeline *);
    VkResult (*p_vkCreateCuFunctionNVX)(VkDevice, const VkCuFunctionCreateInfoNVX *, const VkAllocationCallbacks *, VkCuFunctionNVX *);
    VkResult (*p_vkCreateCuModuleNVX)(VkDevice, const VkCuModuleCreateInfoNVX *, const VkAllocationCallbacks *, VkCuModuleNVX *);
    VkResult (*p_vkCreateCudaFunctionNV)(VkDevice, const VkCudaFunctionCreateInfoNV *, const VkAllocationCallbacks *, VkCudaFunctionNV *);
    VkResult (*p_vkCreateCudaModuleNV)(VkDevice, const VkCudaModuleCreateInfoNV *, const VkAllocationCallbacks *, VkCudaModuleNV *);
    VkResult (*p_vkCreateDeferredOperationKHR)(VkDevice, const VkAllocationCallbacks *, VkDeferredOperationKHR *);
    VkResult (*p_vkCreateDescriptorPool)(VkDevice, const VkDescriptorPoolCreateInfo *, const VkAllocationCallbacks *, VkDescriptorPool *);
    VkResult (*p_vkCreateDescriptorSetLayout)(VkDevice, const VkDescriptorSetLayoutCreateInfo *, const VkAllocationCallbacks *, VkDescriptorSetLayout *);
    VkResult (*p_vkCreateDescriptorUpdateTemplate)(VkDevice, const VkDescriptorUpdateTemplateCreateInfo *, const VkAllocationCallbacks *, VkDescriptorUpdateTemplate *);
    VkResult (*p_vkCreateDescriptorUpdateTemplateKHR)(VkDevice, const VkDescriptorUpdateTemplateCreateInfo *, const VkAllocationCallbacks *, VkDescriptorUpdateTemplate *);
    VkResult (*p_vkCreateEvent)(VkDevice, const VkEventCreateInfo *, const VkAllocationCallbacks *, VkEvent *);
    VkResult (*p_vkCreateFence)(VkDevice, const VkFenceCreateInfo *, const VkAllocationCallbacks *, VkFence *);
    VkResult (*p_vkCreateFramebuffer)(VkDevice, const VkFramebufferCreateInfo *, const VkAllocationCallbacks *, VkFramebuffer *);
    VkResult (*p_vkCreateGraphicsPipelines)(VkDevice, VkPipelineCache, uint32_t, const VkGraphicsPipelineCreateInfo *, const VkAllocationCallbacks *, VkPipeline *);
    VkResult (*p_vkCreateImage)(VkDevice, const VkImageCreateInfo *, const VkAllocationCallbacks *, VkImage *);
    VkResult (*p_vkCreateImageView)(VkDevice, const VkImageViewCreateInfo *, const VkAllocationCallbacks *, VkImageView *);
    VkResult (*p_vkCreateIndirectCommandsLayoutNV)(VkDevice, const VkIndirectCommandsLayoutCreateInfoNV *, const VkAllocationCallbacks *, VkIndirectCommandsLayoutNV *);
    VkResult (*p_vkCreateMicromapEXT)(VkDevice, const VkMicromapCreateInfoEXT *, const VkAllocationCallbacks *, VkMicromapEXT *);
    VkResult (*p_vkCreateOpticalFlowSessionNV)(VkDevice, const VkOpticalFlowSessionCreateInfoNV *, const VkAllocationCallbacks *, VkOpticalFlowSessionNV *);
    VkResult (*p_vkCreatePipelineCache)(VkDevice, const VkPipelineCacheCreateInfo *, const VkAllocationCallbacks *, VkPipelineCache *);
    VkResult (*p_vkCreatePipelineLayout)(VkDevice, const VkPipelineLayoutCreateInfo *, const VkAllocationCallbacks *, VkPipelineLayout *);
    VkResult (*p_vkCreatePrivateDataSlot)(VkDevice, const VkPrivateDataSlotCreateInfo *, const VkAllocationCallbacks *, VkPrivateDataSlot *);
    VkResult (*p_vkCreatePrivateDataSlotEXT)(VkDevice, const VkPrivateDataSlotCreateInfo *, const VkAllocationCallbacks *, VkPrivateDataSlot *);
    VkResult (*p_vkCreateQueryPool)(VkDevice, const VkQueryPoolCreateInfo *, const VkAllocationCallbacks *, VkQueryPool *);
    VkResult (*p_vkCreateRayTracingPipelinesKHR)(VkDevice, VkDeferredOperationKHR, VkPipelineCache, uint32_t, const VkRayTracingPipelineCreateInfoKHR *, const VkAllocationCallbacks *, VkPipeline *);
    VkResult (*p_vkCreateRayTracingPipelinesNV)(VkDevice, VkPipelineCache, uint32_t, const VkRayTracingPipelineCreateInfoNV *, const VkAllocationCallbacks *, VkPipeline *);
    VkResult (*p_vkCreateRenderPass)(VkDevice, const VkRenderPassCreateInfo *, const VkAllocationCallbacks *, VkRenderPass *);
    VkResult (*p_vkCreateRenderPass2)(VkDevice, const VkRenderPassCreateInfo2 *, const VkAllocationCallbacks *, VkRenderPass *);
    VkResult (*p_vkCreateRenderPass2KHR)(VkDevice, const VkRenderPassCreateInfo2 *, const VkAllocationCallbacks *, VkRenderPass *);
    VkResult (*p_vkCreateSampler)(VkDevice, const VkSamplerCreateInfo *, const VkAllocationCallbacks *, VkSampler *);
    VkResult (*p_vkCreateSamplerYcbcrConversion)(VkDevice, const VkSamplerYcbcrConversionCreateInfo *, const VkAllocationCallbacks *, VkSamplerYcbcrConversion *);
    VkResult (*p_vkCreateSamplerYcbcrConversionKHR)(VkDevice, const VkSamplerYcbcrConversionCreateInfo *, const VkAllocationCallbacks *, VkSamplerYcbcrConversion *);
    VkResult (*p_vkCreateSemaphore)(VkDevice, const VkSemaphoreCreateInfo *, const VkAllocationCallbacks *, VkSemaphore *);
    VkResult (*p_vkCreateShaderModule)(VkDevice, const VkShaderModuleCreateInfo *, const VkAllocationCallbacks *, VkShaderModule *);
    VkResult (*p_vkCreateShadersEXT)(VkDevice, uint32_t, const VkShaderCreateInfoEXT *, const VkAllocationCallbacks *, VkShaderEXT *);
    VkResult (*p_vkCreateSwapchainKHR)(VkDevice, const VkSwapchainCreateInfoKHR *, const VkAllocationCallbacks *, VkSwapchainKHR *);
    VkResult (*p_vkCreateValidationCacheEXT)(VkDevice, const VkValidationCacheCreateInfoEXT *, const VkAllocationCallbacks *, VkValidationCacheEXT *);
    VkResult (*p_vkDebugMarkerSetObjectNameEXT)(VkDevice, const VkDebugMarkerObjectNameInfoEXT *);
    VkResult (*p_vkDebugMarkerSetObjectTagEXT)(VkDevice, const VkDebugMarkerObjectTagInfoEXT *);
    VkResult (*p_vkDeferredOperationJoinKHR)(VkDevice, VkDeferredOperationKHR);
    void (*p_vkDestroyAccelerationStructureKHR)(VkDevice, VkAccelerationStructureKHR, const VkAllocationCallbacks *);
    void (*p_vkDestroyAccelerationStructureNV)(VkDevice, VkAccelerationStructureNV, const VkAllocationCallbacks *);
    void (*p_vkDestroyBuffer)(VkDevice, VkBuffer, const VkAllocationCallbacks *);
    void (*p_vkDestroyBufferView)(VkDevice, VkBufferView, const VkAllocationCallbacks *);
    void (*p_vkDestroyCommandPool)(VkDevice, VkCommandPool, const VkAllocationCallbacks *);
    void (*p_vkDestroyCuFunctionNVX)(VkDevice, VkCuFunctionNVX, const VkAllocationCallbacks *);
    void (*p_vkDestroyCuModuleNVX)(VkDevice, VkCuModuleNVX, const VkAllocationCallbacks *);
    void (*p_vkDestroyCudaFunctionNV)(VkDevice, VkCudaFunctionNV, const VkAllocationCallbacks *);
    void (*p_vkDestroyCudaModuleNV)(VkDevice, VkCudaModuleNV, const VkAllocationCallbacks *);
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
    void (*p_vkDestroyShaderEXT)(VkDevice, VkShaderEXT, const VkAllocationCallbacks *);
    void (*p_vkDestroyShaderModule)(VkDevice, VkShaderModule, const VkAllocationCallbacks *);
    void (*p_vkDestroySwapchainKHR)(VkDevice, VkSwapchainKHR, const VkAllocationCallbacks *);
    void (*p_vkDestroyValidationCacheEXT)(VkDevice, VkValidationCacheEXT, const VkAllocationCallbacks *);
    VkResult (*p_vkDeviceWaitIdle)(VkDevice);
    VkResult (*p_vkEndCommandBuffer)(VkCommandBuffer);
    VkResult (*p_vkFlushMappedMemoryRanges)(VkDevice, uint32_t, const VkMappedMemoryRange *);
    void (*p_vkFreeCommandBuffers)(VkDevice, VkCommandPool, uint32_t, const VkCommandBuffer *);
    VkResult (*p_vkFreeDescriptorSets)(VkDevice, VkDescriptorPool, uint32_t, const VkDescriptorSet *);
    void (*p_vkFreeMemory)(VkDevice, VkDeviceMemory, const VkAllocationCallbacks *);
    void (*p_vkGetAccelerationStructureBuildSizesKHR)(VkDevice, VkAccelerationStructureBuildTypeKHR, const VkAccelerationStructureBuildGeometryInfoKHR *, const uint32_t *, VkAccelerationStructureBuildSizesInfoKHR *);
    VkDeviceAddress (*p_vkGetAccelerationStructureDeviceAddressKHR)(VkDevice, const VkAccelerationStructureDeviceAddressInfoKHR *);
    VkResult (*p_vkGetAccelerationStructureHandleNV)(VkDevice, VkAccelerationStructureNV, size_t, void *);
    void (*p_vkGetAccelerationStructureMemoryRequirementsNV)(VkDevice, const VkAccelerationStructureMemoryRequirementsInfoNV *, VkMemoryRequirements2KHR *);
    VkResult (*p_vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT)(VkDevice, const VkAccelerationStructureCaptureDescriptorDataInfoEXT *, void *);
    VkDeviceAddress (*p_vkGetBufferDeviceAddress)(VkDevice, const VkBufferDeviceAddressInfo *);
    VkDeviceAddress (*p_vkGetBufferDeviceAddressEXT)(VkDevice, const VkBufferDeviceAddressInfo *);
    VkDeviceAddress (*p_vkGetBufferDeviceAddressKHR)(VkDevice, const VkBufferDeviceAddressInfo *);
    void (*p_vkGetBufferMemoryRequirements)(VkDevice, VkBuffer, VkMemoryRequirements *);
    void (*p_vkGetBufferMemoryRequirements2)(VkDevice, const VkBufferMemoryRequirementsInfo2 *, VkMemoryRequirements2 *);
    void (*p_vkGetBufferMemoryRequirements2KHR)(VkDevice, const VkBufferMemoryRequirementsInfo2 *, VkMemoryRequirements2 *);
    uint64_t (*p_vkGetBufferOpaqueCaptureAddress)(VkDevice, const VkBufferDeviceAddressInfo *);
    uint64_t (*p_vkGetBufferOpaqueCaptureAddressKHR)(VkDevice, const VkBufferDeviceAddressInfo *);
    VkResult (*p_vkGetBufferOpaqueCaptureDescriptorDataEXT)(VkDevice, const VkBufferCaptureDescriptorDataInfoEXT *, void *);
    VkResult (*p_vkGetCalibratedTimestampsEXT)(VkDevice, uint32_t, const VkCalibratedTimestampInfoKHR *, uint64_t *, uint64_t *);
    VkResult (*p_vkGetCalibratedTimestampsKHR)(VkDevice, uint32_t, const VkCalibratedTimestampInfoKHR *, uint64_t *, uint64_t *);
    VkResult (*p_vkGetCudaModuleCacheNV)(VkDevice, VkCudaModuleNV, size_t *, void *);
    uint32_t (*p_vkGetDeferredOperationMaxConcurrencyKHR)(VkDevice, VkDeferredOperationKHR);
    VkResult (*p_vkGetDeferredOperationResultKHR)(VkDevice, VkDeferredOperationKHR);
    void (*p_vkGetDescriptorEXT)(VkDevice, const VkDescriptorGetInfoEXT *, size_t, void *);
    void (*p_vkGetDescriptorSetHostMappingVALVE)(VkDevice, VkDescriptorSet, void **);
    void (*p_vkGetDescriptorSetLayoutBindingOffsetEXT)(VkDevice, VkDescriptorSetLayout, uint32_t, VkDeviceSize *);
    void (*p_vkGetDescriptorSetLayoutHostMappingInfoVALVE)(VkDevice, const VkDescriptorSetBindingReferenceVALVE *, VkDescriptorSetLayoutHostMappingInfoVALVE *);
    void (*p_vkGetDescriptorSetLayoutSizeEXT)(VkDevice, VkDescriptorSetLayout, VkDeviceSize *);
    void (*p_vkGetDescriptorSetLayoutSupport)(VkDevice, const VkDescriptorSetLayoutCreateInfo *, VkDescriptorSetLayoutSupport *);
    void (*p_vkGetDescriptorSetLayoutSupportKHR)(VkDevice, const VkDescriptorSetLayoutCreateInfo *, VkDescriptorSetLayoutSupport *);
    void (*p_vkGetDeviceAccelerationStructureCompatibilityKHR)(VkDevice, const VkAccelerationStructureVersionInfoKHR *, VkAccelerationStructureCompatibilityKHR *);
    void (*p_vkGetDeviceBufferMemoryRequirements)(VkDevice, const VkDeviceBufferMemoryRequirements *, VkMemoryRequirements2 *);
    void (*p_vkGetDeviceBufferMemoryRequirementsKHR)(VkDevice, const VkDeviceBufferMemoryRequirements *, VkMemoryRequirements2 *);
    VkResult (*p_vkGetDeviceFaultInfoEXT)(VkDevice, VkDeviceFaultCountsEXT *, VkDeviceFaultInfoEXT *);
    void (*p_vkGetDeviceGroupPeerMemoryFeatures)(VkDevice, uint32_t, uint32_t, uint32_t, VkPeerMemoryFeatureFlags *);
    void (*p_vkGetDeviceGroupPeerMemoryFeaturesKHR)(VkDevice, uint32_t, uint32_t, uint32_t, VkPeerMemoryFeatureFlags *);
    VkResult (*p_vkGetDeviceGroupPresentCapabilitiesKHR)(VkDevice, VkDeviceGroupPresentCapabilitiesKHR *);
    VkResult (*p_vkGetDeviceGroupSurfacePresentModesKHR)(VkDevice, VkSurfaceKHR, VkDeviceGroupPresentModeFlagsKHR *);
    void (*p_vkGetDeviceImageMemoryRequirements)(VkDevice, const VkDeviceImageMemoryRequirements *, VkMemoryRequirements2 *);
    void (*p_vkGetDeviceImageMemoryRequirementsKHR)(VkDevice, const VkDeviceImageMemoryRequirements *, VkMemoryRequirements2 *);
    void (*p_vkGetDeviceImageSparseMemoryRequirements)(VkDevice, const VkDeviceImageMemoryRequirements *, uint32_t *, VkSparseImageMemoryRequirements2 *);
    void (*p_vkGetDeviceImageSparseMemoryRequirementsKHR)(VkDevice, const VkDeviceImageMemoryRequirements *, uint32_t *, VkSparseImageMemoryRequirements2 *);
    void (*p_vkGetDeviceImageSubresourceLayoutKHR)(VkDevice, const VkDeviceImageSubresourceInfoKHR *, VkSubresourceLayout2KHR *);
    void (*p_vkGetDeviceMemoryCommitment)(VkDevice, VkDeviceMemory, VkDeviceSize *);
    uint64_t (*p_vkGetDeviceMemoryOpaqueCaptureAddress)(VkDevice, const VkDeviceMemoryOpaqueCaptureAddressInfo *);
    uint64_t (*p_vkGetDeviceMemoryOpaqueCaptureAddressKHR)(VkDevice, const VkDeviceMemoryOpaqueCaptureAddressInfo *);
    void (*p_vkGetDeviceMicromapCompatibilityEXT)(VkDevice, const VkMicromapVersionInfoEXT *, VkAccelerationStructureCompatibilityKHR *);
    void (*p_vkGetDeviceQueue)(VkDevice, uint32_t, uint32_t, VkQueue *);
    void (*p_vkGetDeviceQueue2)(VkDevice, const VkDeviceQueueInfo2 *, VkQueue *);
    VkResult (*p_vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI)(VkDevice, VkRenderPass, VkExtent2D *);
    VkResult (*p_vkGetDynamicRenderingTilePropertiesQCOM)(VkDevice, const VkRenderingInfo *, VkTilePropertiesQCOM *);
    VkResult (*p_vkGetEventStatus)(VkDevice, VkEvent);
    VkResult (*p_vkGetFenceStatus)(VkDevice, VkFence);
    VkResult (*p_vkGetFramebufferTilePropertiesQCOM)(VkDevice, VkFramebuffer, uint32_t *, VkTilePropertiesQCOM *);
    void (*p_vkGetGeneratedCommandsMemoryRequirementsNV)(VkDevice, const VkGeneratedCommandsMemoryRequirementsInfoNV *, VkMemoryRequirements2 *);
    void (*p_vkGetImageMemoryRequirements)(VkDevice, VkImage, VkMemoryRequirements *);
    void (*p_vkGetImageMemoryRequirements2)(VkDevice, const VkImageMemoryRequirementsInfo2 *, VkMemoryRequirements2 *);
    void (*p_vkGetImageMemoryRequirements2KHR)(VkDevice, const VkImageMemoryRequirementsInfo2 *, VkMemoryRequirements2 *);
    VkResult (*p_vkGetImageOpaqueCaptureDescriptorDataEXT)(VkDevice, const VkImageCaptureDescriptorDataInfoEXT *, void *);
    void (*p_vkGetImageSparseMemoryRequirements)(VkDevice, VkImage, uint32_t *, VkSparseImageMemoryRequirements *);
    void (*p_vkGetImageSparseMemoryRequirements2)(VkDevice, const VkImageSparseMemoryRequirementsInfo2 *, uint32_t *, VkSparseImageMemoryRequirements2 *);
    void (*p_vkGetImageSparseMemoryRequirements2KHR)(VkDevice, const VkImageSparseMemoryRequirementsInfo2 *, uint32_t *, VkSparseImageMemoryRequirements2 *);
    void (*p_vkGetImageSubresourceLayout)(VkDevice, VkImage, const VkImageSubresource *, VkSubresourceLayout *);
    void (*p_vkGetImageSubresourceLayout2EXT)(VkDevice, VkImage, const VkImageSubresource2KHR *, VkSubresourceLayout2KHR *);
    void (*p_vkGetImageSubresourceLayout2KHR)(VkDevice, VkImage, const VkImageSubresource2KHR *, VkSubresourceLayout2KHR *);
    VkResult (*p_vkGetImageViewAddressNVX)(VkDevice, VkImageView, VkImageViewAddressPropertiesNVX *);
    uint32_t (*p_vkGetImageViewHandleNVX)(VkDevice, const VkImageViewHandleInfoNVX *);
    VkResult (*p_vkGetImageViewOpaqueCaptureDescriptorDataEXT)(VkDevice, const VkImageViewCaptureDescriptorDataInfoEXT *, void *);
    void (*p_vkGetLatencyTimingsNV)(VkDevice, VkSwapchainKHR, VkGetLatencyMarkerInfoNV *);
    VkResult (*p_vkGetMemoryHostPointerPropertiesEXT)(VkDevice, VkExternalMemoryHandleTypeFlagBits, const void *, VkMemoryHostPointerPropertiesEXT *);
    void (*p_vkGetMicromapBuildSizesEXT)(VkDevice, VkAccelerationStructureBuildTypeKHR, const VkMicromapBuildInfoEXT *, VkMicromapBuildSizesInfoEXT *);
    VkResult (*p_vkGetPerformanceParameterINTEL)(VkDevice, VkPerformanceParameterTypeINTEL, VkPerformanceValueINTEL *);
    VkResult (*p_vkGetPipelineCacheData)(VkDevice, VkPipelineCache, size_t *, void *);
    VkResult (*p_vkGetPipelineExecutableInternalRepresentationsKHR)(VkDevice, const VkPipelineExecutableInfoKHR *, uint32_t *, VkPipelineExecutableInternalRepresentationKHR *);
    VkResult (*p_vkGetPipelineExecutablePropertiesKHR)(VkDevice, const VkPipelineInfoKHR *, uint32_t *, VkPipelineExecutablePropertiesKHR *);
    VkResult (*p_vkGetPipelineExecutableStatisticsKHR)(VkDevice, const VkPipelineExecutableInfoKHR *, uint32_t *, VkPipelineExecutableStatisticKHR *);
    VkDeviceAddress (*p_vkGetPipelineIndirectDeviceAddressNV)(VkDevice, const VkPipelineIndirectDeviceAddressInfoNV *);
    void (*p_vkGetPipelineIndirectMemoryRequirementsNV)(VkDevice, const VkComputePipelineCreateInfo *, VkMemoryRequirements2 *);
    VkResult (*p_vkGetPipelinePropertiesEXT)(VkDevice, const VkPipelineInfoEXT *, VkBaseOutStructure *);
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
    void (*p_vkGetRenderingAreaGranularityKHR)(VkDevice, const VkRenderingAreaInfoKHR *, VkExtent2D *);
    VkResult (*p_vkGetSamplerOpaqueCaptureDescriptorDataEXT)(VkDevice, const VkSamplerCaptureDescriptorDataInfoEXT *, void *);
    VkResult (*p_vkGetSemaphoreCounterValue)(VkDevice, VkSemaphore, uint64_t *);
    VkResult (*p_vkGetSemaphoreCounterValueKHR)(VkDevice, VkSemaphore, uint64_t *);
    VkResult (*p_vkGetShaderBinaryDataEXT)(VkDevice, VkShaderEXT, size_t *, void *);
    VkResult (*p_vkGetShaderInfoAMD)(VkDevice, VkPipeline, VkShaderStageFlagBits, VkShaderInfoTypeAMD, size_t *, void *);
    void (*p_vkGetShaderModuleCreateInfoIdentifierEXT)(VkDevice, const VkShaderModuleCreateInfo *, VkShaderModuleIdentifierEXT *);
    void (*p_vkGetShaderModuleIdentifierEXT)(VkDevice, VkShaderModule, VkShaderModuleIdentifierEXT *);
    VkResult (*p_vkGetSwapchainImagesKHR)(VkDevice, VkSwapchainKHR, uint32_t *, VkImage *);
    VkResult (*p_vkGetValidationCacheDataEXT)(VkDevice, VkValidationCacheEXT, size_t *, void *);
    VkResult (*p_vkInitializePerformanceApiINTEL)(VkDevice, const VkInitializePerformanceApiInfoINTEL *);
    VkResult (*p_vkInvalidateMappedMemoryRanges)(VkDevice, uint32_t, const VkMappedMemoryRange *);
    VkResult (*p_vkLatencySleepNV)(VkDevice, VkSwapchainKHR, const VkLatencySleepInfoNV *);
    VkResult (*p_vkMapMemory)(VkDevice, VkDeviceMemory, VkDeviceSize, VkDeviceSize, VkMemoryMapFlags, void **);
    VkResult (*p_vkMapMemory2KHR)(VkDevice, const VkMemoryMapInfoKHR *, void **);
    VkResult (*p_vkMergePipelineCaches)(VkDevice, VkPipelineCache, uint32_t, const VkPipelineCache *);
    VkResult (*p_vkMergeValidationCachesEXT)(VkDevice, VkValidationCacheEXT, uint32_t, const VkValidationCacheEXT *);
    void (*p_vkQueueBeginDebugUtilsLabelEXT)(VkQueue, const VkDebugUtilsLabelEXT *);
    VkResult (*p_vkQueueBindSparse)(VkQueue, uint32_t, const VkBindSparseInfo *, VkFence);
    void (*p_vkQueueEndDebugUtilsLabelEXT)(VkQueue);
    void (*p_vkQueueInsertDebugUtilsLabelEXT)(VkQueue, const VkDebugUtilsLabelEXT *);
    void (*p_vkQueueNotifyOutOfBandNV)(VkQueue, const VkOutOfBandQueueTypeInfoNV *);
    VkResult (*p_vkQueuePresentKHR)(VkQueue, const VkPresentInfoKHR *);
    VkResult (*p_vkQueueSetPerformanceConfigurationINTEL)(VkQueue, VkPerformanceConfigurationINTEL);
    VkResult (*p_vkQueueSubmit)(VkQueue, uint32_t, const VkSubmitInfo *, VkFence);
    VkResult (*p_vkQueueSubmit2)(VkQueue, uint32_t, const VkSubmitInfo2 *, VkFence);
    VkResult (*p_vkQueueSubmit2KHR)(VkQueue, uint32_t, const VkSubmitInfo2 *, VkFence);
    VkResult (*p_vkQueueWaitIdle)(VkQueue);
    VkResult (*p_vkReleasePerformanceConfigurationINTEL)(VkDevice, VkPerformanceConfigurationINTEL);
    void (*p_vkReleaseProfilingLockKHR)(VkDevice);
    VkResult (*p_vkReleaseSwapchainImagesEXT)(VkDevice, const VkReleaseSwapchainImagesInfoEXT *);
    VkResult (*p_vkResetCommandBuffer)(VkCommandBuffer, VkCommandBufferResetFlags);
    VkResult (*p_vkResetCommandPool)(VkDevice, VkCommandPool, VkCommandPoolResetFlags);
    VkResult (*p_vkResetDescriptorPool)(VkDevice, VkDescriptorPool, VkDescriptorPoolResetFlags);
    VkResult (*p_vkResetEvent)(VkDevice, VkEvent);
    VkResult (*p_vkResetFences)(VkDevice, uint32_t, const VkFence *);
    void (*p_vkResetQueryPool)(VkDevice, VkQueryPool, uint32_t, uint32_t);
    void (*p_vkResetQueryPoolEXT)(VkDevice, VkQueryPool, uint32_t, uint32_t);
    VkResult (*p_vkSetDebugUtilsObjectNameEXT)(VkDevice, const VkDebugUtilsObjectNameInfoEXT *);
    VkResult (*p_vkSetDebugUtilsObjectTagEXT)(VkDevice, const VkDebugUtilsObjectTagInfoEXT *);
    void (*p_vkSetDeviceMemoryPriorityEXT)(VkDevice, VkDeviceMemory, float);
    VkResult (*p_vkSetEvent)(VkDevice, VkEvent);
    void (*p_vkSetHdrMetadataEXT)(VkDevice, uint32_t, const VkSwapchainKHR *, const VkHdrMetadataEXT *);
    void (*p_vkSetLatencyMarkerNV)(VkDevice, VkSwapchainKHR, const VkSetLatencyMarkerInfoNV *);
    VkResult (*p_vkSetLatencySleepModeNV)(VkDevice, VkSwapchainKHR, const VkLatencySleepModeInfoNV *);
    VkResult (*p_vkSetPrivateData)(VkDevice, VkObjectType, uint64_t, VkPrivateDataSlot, uint64_t);
    VkResult (*p_vkSetPrivateDataEXT)(VkDevice, VkObjectType, uint64_t, VkPrivateDataSlot, uint64_t);
    VkResult (*p_vkSignalSemaphore)(VkDevice, const VkSemaphoreSignalInfo *);
    VkResult (*p_vkSignalSemaphoreKHR)(VkDevice, const VkSemaphoreSignalInfo *);
    VkResult (*p_vkTransitionImageLayoutEXT)(VkDevice, uint32_t, const VkHostImageLayoutTransitionInfoEXT *);
    void (*p_vkTrimCommandPool)(VkDevice, VkCommandPool, VkCommandPoolTrimFlags);
    void (*p_vkTrimCommandPoolKHR)(VkDevice, VkCommandPool, VkCommandPoolTrimFlags);
    void (*p_vkUninitializePerformanceApiINTEL)(VkDevice);
    void (*p_vkUnmapMemory)(VkDevice, VkDeviceMemory);
    VkResult (*p_vkUnmapMemory2KHR)(VkDevice, const VkMemoryUnmapInfoKHR *);
    void (*p_vkUpdateDescriptorSetWithTemplate)(VkDevice, VkDescriptorSet, VkDescriptorUpdateTemplate, const void *);
    void (*p_vkUpdateDescriptorSetWithTemplateKHR)(VkDevice, VkDescriptorSet, VkDescriptorUpdateTemplate, const void *);
    void (*p_vkUpdateDescriptorSets)(VkDevice, uint32_t, const VkWriteDescriptorSet *, uint32_t, const VkCopyDescriptorSet *);
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
    void (*p_vkSubmitDebugUtilsMessageEXT)(VkInstance, VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT *);
    VkResult (*p_vkCreateDevice)(VkPhysicalDevice, const VkDeviceCreateInfo *, const VkAllocationCallbacks *, VkDevice *);
    VkResult (*p_vkEnumerateDeviceExtensionProperties)(VkPhysicalDevice, const char *, uint32_t *, VkExtensionProperties *);
    VkResult (*p_vkEnumerateDeviceLayerProperties)(VkPhysicalDevice, uint32_t *, VkLayerProperties *);
    VkResult (*p_vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR)(VkPhysicalDevice, uint32_t, uint32_t *, VkPerformanceCounterKHR *, VkPerformanceCounterDescriptionKHR *);
    VkResult (*p_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT)(VkPhysicalDevice, uint32_t *, VkTimeDomainKHR *);
    VkResult (*p_vkGetPhysicalDeviceCalibrateableTimeDomainsKHR)(VkPhysicalDevice, uint32_t *, VkTimeDomainKHR *);
    VkResult (*p_vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR)(VkPhysicalDevice, uint32_t *, VkCooperativeMatrixPropertiesKHR *);
    VkResult (*p_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV)(VkPhysicalDevice, uint32_t *, VkCooperativeMatrixPropertiesNV *);
    void (*p_vkGetPhysicalDeviceFeatures)(VkPhysicalDevice, VkPhysicalDeviceFeatures *);
    void (*p_vkGetPhysicalDeviceFeatures2)(VkPhysicalDevice, VkPhysicalDeviceFeatures2 *);
    void (*p_vkGetPhysicalDeviceFeatures2KHR)(VkPhysicalDevice, VkPhysicalDeviceFeatures2 *);
    void (*p_vkGetPhysicalDeviceFormatProperties)(VkPhysicalDevice, VkFormat, VkFormatProperties *);
    void (*p_vkGetPhysicalDeviceFormatProperties2)(VkPhysicalDevice, VkFormat, VkFormatProperties2 *);
    void (*p_vkGetPhysicalDeviceFormatProperties2KHR)(VkPhysicalDevice, VkFormat, VkFormatProperties2 *);
    VkResult (*p_vkGetPhysicalDeviceFragmentShadingRatesKHR)(VkPhysicalDevice, uint32_t *, VkPhysicalDeviceFragmentShadingRateKHR *);
    VkResult (*p_vkGetPhysicalDeviceImageFormatProperties)(VkPhysicalDevice, VkFormat, VkImageType, VkImageTiling, VkImageUsageFlags, VkImageCreateFlags, VkImageFormatProperties *);
    VkResult (*p_vkGetPhysicalDeviceImageFormatProperties2)(VkPhysicalDevice, const VkPhysicalDeviceImageFormatInfo2 *, VkImageFormatProperties2 *);
    VkResult (*p_vkGetPhysicalDeviceImageFormatProperties2KHR)(VkPhysicalDevice, const VkPhysicalDeviceImageFormatInfo2 *, VkImageFormatProperties2 *);
    void (*p_vkGetPhysicalDeviceMemoryProperties)(VkPhysicalDevice, VkPhysicalDeviceMemoryProperties *);
    void (*p_vkGetPhysicalDeviceMemoryProperties2)(VkPhysicalDevice, VkPhysicalDeviceMemoryProperties2 *);
    void (*p_vkGetPhysicalDeviceMemoryProperties2KHR)(VkPhysicalDevice, VkPhysicalDeviceMemoryProperties2 *);
    void (*p_vkGetPhysicalDeviceMultisamplePropertiesEXT)(VkPhysicalDevice, VkSampleCountFlagBits, VkMultisamplePropertiesEXT *);
    VkResult (*p_vkGetPhysicalDeviceOpticalFlowImageFormatsNV)(VkPhysicalDevice, const VkOpticalFlowImageFormatInfoNV *, uint32_t *, VkOpticalFlowImageFormatPropertiesNV *);
    VkResult (*p_vkGetPhysicalDevicePresentRectanglesKHR)(VkPhysicalDevice, VkSurfaceKHR, uint32_t *, VkRect2D *);
    void (*p_vkGetPhysicalDeviceProperties)(VkPhysicalDevice, VkPhysicalDeviceProperties *);
    void (*p_vkGetPhysicalDeviceProperties2)(VkPhysicalDevice, VkPhysicalDeviceProperties2 *);
    void (*p_vkGetPhysicalDeviceProperties2KHR)(VkPhysicalDevice, VkPhysicalDeviceProperties2 *);
    void (*p_vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR)(VkPhysicalDevice, const VkQueryPoolPerformanceCreateInfoKHR *, uint32_t *);
    void (*p_vkGetPhysicalDeviceQueueFamilyProperties)(VkPhysicalDevice, uint32_t *, VkQueueFamilyProperties *);
    void (*p_vkGetPhysicalDeviceQueueFamilyProperties2)(VkPhysicalDevice, uint32_t *, VkQueueFamilyProperties2 *);
    void (*p_vkGetPhysicalDeviceQueueFamilyProperties2KHR)(VkPhysicalDevice, uint32_t *, VkQueueFamilyProperties2 *);
    void (*p_vkGetPhysicalDeviceSparseImageFormatProperties)(VkPhysicalDevice, VkFormat, VkImageType, VkSampleCountFlagBits, VkImageUsageFlags, VkImageTiling, uint32_t *, VkSparseImageFormatProperties *);
    void (*p_vkGetPhysicalDeviceSparseImageFormatProperties2)(VkPhysicalDevice, const VkPhysicalDeviceSparseImageFormatInfo2 *, uint32_t *, VkSparseImageFormatProperties2 *);
    void (*p_vkGetPhysicalDeviceSparseImageFormatProperties2KHR)(VkPhysicalDevice, const VkPhysicalDeviceSparseImageFormatInfo2 *, uint32_t *, VkSparseImageFormatProperties2 *);
    VkResult (*p_vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV)(VkPhysicalDevice, uint32_t *, VkFramebufferMixedSamplesCombinationNV *);
    VkResult (*p_vkGetPhysicalDeviceSurfaceCapabilities2KHR)(VkPhysicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR *, VkSurfaceCapabilities2KHR *);
    VkResult (*p_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)(VkPhysicalDevice, VkSurfaceKHR, VkSurfaceCapabilitiesKHR *);
    VkResult (*p_vkGetPhysicalDeviceSurfaceFormats2KHR)(VkPhysicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR *, uint32_t *, VkSurfaceFormat2KHR *);
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
    USE_VK_FUNC(vkCmdBindDescriptorBufferEmbeddedSamplers2EXT) \
    USE_VK_FUNC(vkCmdBindDescriptorBufferEmbeddedSamplersEXT) \
    USE_VK_FUNC(vkCmdBindDescriptorBuffersEXT) \
    USE_VK_FUNC(vkCmdBindDescriptorSets) \
    USE_VK_FUNC(vkCmdBindDescriptorSets2KHR) \
    USE_VK_FUNC(vkCmdBindIndexBuffer) \
    USE_VK_FUNC(vkCmdBindIndexBuffer2KHR) \
    USE_VK_FUNC(vkCmdBindInvocationMaskHUAWEI) \
    USE_VK_FUNC(vkCmdBindPipeline) \
    USE_VK_FUNC(vkCmdBindPipelineShaderGroupNV) \
    USE_VK_FUNC(vkCmdBindShadersEXT) \
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
    USE_VK_FUNC(vkCmdCopyMemoryIndirectNV) \
    USE_VK_FUNC(vkCmdCopyMemoryToAccelerationStructureKHR) \
    USE_VK_FUNC(vkCmdCopyMemoryToImageIndirectNV) \
    USE_VK_FUNC(vkCmdCopyMemoryToMicromapEXT) \
    USE_VK_FUNC(vkCmdCopyMicromapEXT) \
    USE_VK_FUNC(vkCmdCopyMicromapToMemoryEXT) \
    USE_VK_FUNC(vkCmdCopyQueryPoolResults) \
    USE_VK_FUNC(vkCmdCuLaunchKernelNVX) \
    USE_VK_FUNC(vkCmdCudaLaunchKernelNV) \
    USE_VK_FUNC(vkCmdDebugMarkerBeginEXT) \
    USE_VK_FUNC(vkCmdDebugMarkerEndEXT) \
    USE_VK_FUNC(vkCmdDebugMarkerInsertEXT) \
    USE_VK_FUNC(vkCmdDecompressMemoryIndirectCountNV) \
    USE_VK_FUNC(vkCmdDecompressMemoryNV) \
    USE_VK_FUNC(vkCmdDispatch) \
    USE_VK_FUNC(vkCmdDispatchBase) \
    USE_VK_FUNC(vkCmdDispatchBaseKHR) \
    USE_VK_FUNC(vkCmdDispatchIndirect) \
    USE_VK_FUNC(vkCmdDraw) \
    USE_VK_FUNC(vkCmdDrawClusterHUAWEI) \
    USE_VK_FUNC(vkCmdDrawClusterIndirectHUAWEI) \
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
    USE_VK_FUNC(vkCmdPushConstants2KHR) \
    USE_VK_FUNC(vkCmdPushDescriptorSet2KHR) \
    USE_VK_FUNC(vkCmdPushDescriptorSetKHR) \
    USE_VK_FUNC(vkCmdPushDescriptorSetWithTemplate2KHR) \
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
    USE_VK_FUNC(vkCmdSetAttachmentFeedbackLoopEnableEXT) \
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
    USE_VK_FUNC(vkCmdSetDepthBias2EXT) \
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
    USE_VK_FUNC(vkCmdSetDescriptorBufferOffsets2EXT) \
    USE_VK_FUNC(vkCmdSetDescriptorBufferOffsetsEXT) \
    USE_VK_FUNC(vkCmdSetDeviceMask) \
    USE_VK_FUNC(vkCmdSetDeviceMaskKHR) \
    USE_VK_FUNC(vkCmdSetDiscardRectangleEXT) \
    USE_VK_FUNC(vkCmdSetDiscardRectangleEnableEXT) \
    USE_VK_FUNC(vkCmdSetDiscardRectangleModeEXT) \
    USE_VK_FUNC(vkCmdSetEvent) \
    USE_VK_FUNC(vkCmdSetEvent2) \
    USE_VK_FUNC(vkCmdSetEvent2KHR) \
    USE_VK_FUNC(vkCmdSetExclusiveScissorEnableNV) \
    USE_VK_FUNC(vkCmdSetExclusiveScissorNV) \
    USE_VK_FUNC(vkCmdSetExtraPrimitiveOverestimationSizeEXT) \
    USE_VK_FUNC(vkCmdSetFragmentShadingRateEnumNV) \
    USE_VK_FUNC(vkCmdSetFragmentShadingRateKHR) \
    USE_VK_FUNC(vkCmdSetFrontFace) \
    USE_VK_FUNC(vkCmdSetFrontFaceEXT) \
    USE_VK_FUNC(vkCmdSetLineRasterizationModeEXT) \
    USE_VK_FUNC(vkCmdSetLineStippleEXT) \
    USE_VK_FUNC(vkCmdSetLineStippleEnableEXT) \
    USE_VK_FUNC(vkCmdSetLineStippleKHR) \
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
    USE_VK_FUNC(vkCmdSetRenderingAttachmentLocationsKHR) \
    USE_VK_FUNC(vkCmdSetRenderingInputAttachmentIndicesKHR) \
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
    USE_VK_FUNC(vkCmdUpdatePipelineIndirectBufferNV) \
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
    USE_VK_FUNC(vkCopyImageToImageEXT) \
    USE_VK_FUNC(vkCopyImageToMemoryEXT) \
    USE_VK_FUNC(vkCopyMemoryToAccelerationStructureKHR) \
    USE_VK_FUNC(vkCopyMemoryToImageEXT) \
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
    USE_VK_FUNC(vkCreateCudaFunctionNV) \
    USE_VK_FUNC(vkCreateCudaModuleNV) \
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
    USE_VK_FUNC(vkCreateShadersEXT) \
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
    USE_VK_FUNC(vkDestroyCudaFunctionNV) \
    USE_VK_FUNC(vkDestroyCudaModuleNV) \
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
    USE_VK_FUNC(vkDestroyShaderEXT) \
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
    USE_VK_FUNC(vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT) \
    USE_VK_FUNC(vkGetBufferDeviceAddress) \
    USE_VK_FUNC(vkGetBufferDeviceAddressEXT) \
    USE_VK_FUNC(vkGetBufferDeviceAddressKHR) \
    USE_VK_FUNC(vkGetBufferMemoryRequirements) \
    USE_VK_FUNC(vkGetBufferMemoryRequirements2) \
    USE_VK_FUNC(vkGetBufferMemoryRequirements2KHR) \
    USE_VK_FUNC(vkGetBufferOpaqueCaptureAddress) \
    USE_VK_FUNC(vkGetBufferOpaqueCaptureAddressKHR) \
    USE_VK_FUNC(vkGetBufferOpaqueCaptureDescriptorDataEXT) \
    USE_VK_FUNC(vkGetCalibratedTimestampsEXT) \
    USE_VK_FUNC(vkGetCalibratedTimestampsKHR) \
    USE_VK_FUNC(vkGetCudaModuleCacheNV) \
    USE_VK_FUNC(vkGetDeferredOperationMaxConcurrencyKHR) \
    USE_VK_FUNC(vkGetDeferredOperationResultKHR) \
    USE_VK_FUNC(vkGetDescriptorEXT) \
    USE_VK_FUNC(vkGetDescriptorSetHostMappingVALVE) \
    USE_VK_FUNC(vkGetDescriptorSetLayoutBindingOffsetEXT) \
    USE_VK_FUNC(vkGetDescriptorSetLayoutHostMappingInfoVALVE) \
    USE_VK_FUNC(vkGetDescriptorSetLayoutSizeEXT) \
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
    USE_VK_FUNC(vkGetDeviceImageSubresourceLayoutKHR) \
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
    USE_VK_FUNC(vkGetImageOpaqueCaptureDescriptorDataEXT) \
    USE_VK_FUNC(vkGetImageSparseMemoryRequirements) \
    USE_VK_FUNC(vkGetImageSparseMemoryRequirements2) \
    USE_VK_FUNC(vkGetImageSparseMemoryRequirements2KHR) \
    USE_VK_FUNC(vkGetImageSubresourceLayout) \
    USE_VK_FUNC(vkGetImageSubresourceLayout2EXT) \
    USE_VK_FUNC(vkGetImageSubresourceLayout2KHR) \
    USE_VK_FUNC(vkGetImageViewAddressNVX) \
    USE_VK_FUNC(vkGetImageViewHandleNVX) \
    USE_VK_FUNC(vkGetImageViewOpaqueCaptureDescriptorDataEXT) \
    USE_VK_FUNC(vkGetLatencyTimingsNV) \
    USE_VK_FUNC(vkGetMemoryHostPointerPropertiesEXT) \
    USE_VK_FUNC(vkGetMicromapBuildSizesEXT) \
    USE_VK_FUNC(vkGetPerformanceParameterINTEL) \
    USE_VK_FUNC(vkGetPipelineCacheData) \
    USE_VK_FUNC(vkGetPipelineExecutableInternalRepresentationsKHR) \
    USE_VK_FUNC(vkGetPipelineExecutablePropertiesKHR) \
    USE_VK_FUNC(vkGetPipelineExecutableStatisticsKHR) \
    USE_VK_FUNC(vkGetPipelineIndirectDeviceAddressNV) \
    USE_VK_FUNC(vkGetPipelineIndirectMemoryRequirementsNV) \
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
    USE_VK_FUNC(vkGetRenderingAreaGranularityKHR) \
    USE_VK_FUNC(vkGetSamplerOpaqueCaptureDescriptorDataEXT) \
    USE_VK_FUNC(vkGetSemaphoreCounterValue) \
    USE_VK_FUNC(vkGetSemaphoreCounterValueKHR) \
    USE_VK_FUNC(vkGetShaderBinaryDataEXT) \
    USE_VK_FUNC(vkGetShaderInfoAMD) \
    USE_VK_FUNC(vkGetShaderModuleCreateInfoIdentifierEXT) \
    USE_VK_FUNC(vkGetShaderModuleIdentifierEXT) \
    USE_VK_FUNC(vkGetSwapchainImagesKHR) \
    USE_VK_FUNC(vkGetValidationCacheDataEXT) \
    USE_VK_FUNC(vkInitializePerformanceApiINTEL) \
    USE_VK_FUNC(vkInvalidateMappedMemoryRanges) \
    USE_VK_FUNC(vkLatencySleepNV) \
    USE_VK_FUNC(vkMapMemory) \
    USE_VK_FUNC(vkMapMemory2KHR) \
    USE_VK_FUNC(vkMergePipelineCaches) \
    USE_VK_FUNC(vkMergeValidationCachesEXT) \
    USE_VK_FUNC(vkQueueBeginDebugUtilsLabelEXT) \
    USE_VK_FUNC(vkQueueBindSparse) \
    USE_VK_FUNC(vkQueueEndDebugUtilsLabelEXT) \
    USE_VK_FUNC(vkQueueInsertDebugUtilsLabelEXT) \
    USE_VK_FUNC(vkQueueNotifyOutOfBandNV) \
    USE_VK_FUNC(vkQueuePresentKHR) \
    USE_VK_FUNC(vkQueueSetPerformanceConfigurationINTEL) \
    USE_VK_FUNC(vkQueueSubmit) \
    USE_VK_FUNC(vkQueueSubmit2) \
    USE_VK_FUNC(vkQueueSubmit2KHR) \
    USE_VK_FUNC(vkQueueWaitIdle) \
    USE_VK_FUNC(vkReleasePerformanceConfigurationINTEL) \
    USE_VK_FUNC(vkReleaseProfilingLockKHR) \
    USE_VK_FUNC(vkReleaseSwapchainImagesEXT) \
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
    USE_VK_FUNC(vkSetHdrMetadataEXT) \
    USE_VK_FUNC(vkSetLatencyMarkerNV) \
    USE_VK_FUNC(vkSetLatencySleepModeNV) \
    USE_VK_FUNC(vkSetPrivateData) \
    USE_VK_FUNC(vkSetPrivateDataEXT) \
    USE_VK_FUNC(vkSignalSemaphore) \
    USE_VK_FUNC(vkSignalSemaphoreKHR) \
    USE_VK_FUNC(vkTransitionImageLayoutEXT) \
    USE_VK_FUNC(vkTrimCommandPool) \
    USE_VK_FUNC(vkTrimCommandPoolKHR) \
    USE_VK_FUNC(vkUninitializePerformanceApiINTEL) \
    USE_VK_FUNC(vkUnmapMemory) \
    USE_VK_FUNC(vkUnmapMemory2KHR) \
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
    USE_VK_FUNC(vkGetPhysicalDeviceCalibrateableTimeDomainsKHR) \
    USE_VK_FUNC(vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR) \
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
