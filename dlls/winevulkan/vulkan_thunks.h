/* Automatically generated from Vulkan vk.xml and video.xml; DO NOT EDIT!
 *
 * This file is generated from Vulkan vk.xml file covered
 * by the following copyright and permission notice:
 *
 * Copyright 2015-2024 The Khronos Group Inc.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * and from Vulkan video.xml file covered
 * by the following copyright and permission notice:
 *
 * Copyright 2021-2024 The Khronos Group Inc.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __WINE_VULKAN_THUNKS_H
#define __WINE_VULKAN_THUNKS_H

#define WINE_VK_VERSION VK_API_VERSION_1_3

/* Functions for which we have custom implementations outside of the thunks. */
VkResult wine_vkAcquireNextImage2KHR(VkDevice device, const VkAcquireNextImageInfoKHR *pAcquireInfo, uint32_t *pImageIndex);
VkResult wine_vkAcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t *pImageIndex);
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
VkResult wine_vkCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSwapchainKHR *pSwapchain);
VkResult wine_vkCreateWin32SurfaceKHR(VkInstance instance, const VkWin32SurfaceCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface);
void wine_vkDestroyCommandPool(VkDevice device, VkCommandPool commandPool, const VkAllocationCallbacks *pAllocator);
void wine_vkDestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks *pAllocator);
void wine_vkDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks *pAllocator);
void wine_vkDestroyDeferredOperationKHR(VkDevice device, VkDeferredOperationKHR operation, const VkAllocationCallbacks *pAllocator);
void wine_vkDestroyDevice(VkDevice device, const VkAllocationCallbacks *pAllocator);
void wine_vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks *pAllocator);
void wine_vkDestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks *pAllocator);
void wine_vkDestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks *pAllocator);
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
VkResult wine_vkGetPhysicalDevicePresentRectanglesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t *pRectCount, VkRect2D *pRects);
VkResult wine_vkGetPhysicalDeviceSurfaceCapabilities2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR *pSurfaceInfo, VkSurfaceCapabilities2KHR *pSurfaceCapabilities);
VkResult wine_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR *pSurfaceCapabilities);
VkResult wine_vkGetPhysicalDeviceSurfaceFormats2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR *pSurfaceInfo, uint32_t *pSurfaceFormatCount, VkSurfaceFormat2KHR *pSurfaceFormats);
VkResult wine_vkGetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t *pSurfaceFormatCount, VkSurfaceFormatKHR *pSurfaceFormats);
VkResult wine_vkMapMemory(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void **ppData);
VkResult wine_vkMapMemory2KHR(VkDevice device, const VkMemoryMapInfoKHR *pMemoryMapInfo, void **ppData);
VkResult wine_vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR *pPresentInfo);
void wine_vkUnmapMemory(VkDevice device, VkDeviceMemory memory);
VkResult wine_vkUnmapMemory2KHR(VkDevice device, const VkMemoryUnmapInfoKHR *pMemoryUnmapInfo);

#define ALL_VK_DEVICE_FUNCS() \
    USE_VK_FUNC(vkAcquireNextImage2KHR) \
    USE_VK_FUNC(vkAcquireNextImageKHR) \
    USE_VK_FUNC(vkAcquirePerformanceConfigurationINTEL) \
    USE_VK_FUNC(vkAcquireProfilingLockKHR) \
    USE_VK_FUNC(vkAllocateCommandBuffers) \
    USE_VK_FUNC(vkAllocateDescriptorSets) \
    USE_VK_FUNC(vkAllocateMemory) \
    USE_VK_FUNC(vkAntiLagUpdateAMD) \
    USE_VK_FUNC(vkBeginCommandBuffer) \
    USE_VK_FUNC(vkBindAccelerationStructureMemoryNV) \
    USE_VK_FUNC(vkBindBufferMemory) \
    USE_VK_FUNC(vkBindBufferMemory2) \
    USE_VK_FUNC(vkBindBufferMemory2KHR) \
    USE_VK_FUNC(vkBindImageMemory) \
    USE_VK_FUNC(vkBindImageMemory2) \
    USE_VK_FUNC(vkBindImageMemory2KHR) \
    USE_VK_FUNC(vkBindOpticalFlowSessionImageNV) \
    USE_VK_FUNC(vkBindVideoSessionMemoryKHR) \
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
    USE_VK_FUNC(vkCmdBeginVideoCodingKHR) \
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
    USE_VK_FUNC(vkCmdControlVideoCodingKHR) \
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
    USE_VK_FUNC(vkCmdDecodeVideoKHR) \
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
    USE_VK_FUNC(vkCmdEncodeVideoKHR) \
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
    USE_VK_FUNC(vkCmdEndVideoCodingKHR) \
    USE_VK_FUNC(vkCmdExecuteCommands) \
    USE_VK_FUNC(vkCmdExecuteGeneratedCommandsEXT) \
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
    USE_VK_FUNC(vkCmdPreprocessGeneratedCommandsEXT) \
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
    USE_VK_FUNC(vkCmdSetDepthClampRangeEXT) \
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
    USE_VK_FUNC(vkCreateIndirectCommandsLayoutEXT) \
    USE_VK_FUNC(vkCreateIndirectCommandsLayoutNV) \
    USE_VK_FUNC(vkCreateIndirectExecutionSetEXT) \
    USE_VK_FUNC(vkCreateMicromapEXT) \
    USE_VK_FUNC(vkCreateOpticalFlowSessionNV) \
    USE_VK_FUNC(vkCreatePipelineBinariesKHR) \
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
    USE_VK_FUNC(vkCreateVideoSessionKHR) \
    USE_VK_FUNC(vkCreateVideoSessionParametersKHR) \
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
    USE_VK_FUNC(vkDestroyIndirectCommandsLayoutEXT) \
    USE_VK_FUNC(vkDestroyIndirectCommandsLayoutNV) \
    USE_VK_FUNC(vkDestroyIndirectExecutionSetEXT) \
    USE_VK_FUNC(vkDestroyMicromapEXT) \
    USE_VK_FUNC(vkDestroyOpticalFlowSessionNV) \
    USE_VK_FUNC(vkDestroyPipeline) \
    USE_VK_FUNC(vkDestroyPipelineBinaryKHR) \
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
    USE_VK_FUNC(vkDestroyVideoSessionKHR) \
    USE_VK_FUNC(vkDestroyVideoSessionParametersKHR) \
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
    USE_VK_FUNC(vkGetEncodedVideoSessionParametersKHR) \
    USE_VK_FUNC(vkGetEventStatus) \
    USE_VK_FUNC(vkGetFenceStatus) \
    USE_VK_FUNC(vkGetFramebufferTilePropertiesQCOM) \
    USE_VK_FUNC(vkGetGeneratedCommandsMemoryRequirementsEXT) \
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
    USE_VK_FUNC(vkGetImageViewHandle64NVX) \
    USE_VK_FUNC(vkGetImageViewHandleNVX) \
    USE_VK_FUNC(vkGetImageViewOpaqueCaptureDescriptorDataEXT) \
    USE_VK_FUNC(vkGetLatencyTimingsNV) \
    USE_VK_FUNC(vkGetMemoryHostPointerPropertiesEXT) \
    USE_VK_FUNC(vkGetMicromapBuildSizesEXT) \
    USE_VK_FUNC(vkGetPerformanceParameterINTEL) \
    USE_VK_FUNC(vkGetPipelineBinaryDataKHR) \
    USE_VK_FUNC(vkGetPipelineCacheData) \
    USE_VK_FUNC(vkGetPipelineExecutableInternalRepresentationsKHR) \
    USE_VK_FUNC(vkGetPipelineExecutablePropertiesKHR) \
    USE_VK_FUNC(vkGetPipelineExecutableStatisticsKHR) \
    USE_VK_FUNC(vkGetPipelineIndirectDeviceAddressNV) \
    USE_VK_FUNC(vkGetPipelineIndirectMemoryRequirementsNV) \
    USE_VK_FUNC(vkGetPipelineKeyKHR) \
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
    USE_VK_FUNC(vkGetVideoSessionMemoryRequirementsKHR) \
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
    USE_VK_FUNC(vkReleaseCapturedPipelineDataKHR) \
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
    USE_VK_FUNC(vkUpdateIndirectExecutionSetPipelineEXT) \
    USE_VK_FUNC(vkUpdateIndirectExecutionSetShaderEXT) \
    USE_VK_FUNC(vkUpdateVideoSessionParametersKHR) \
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
    USE_VK_FUNC(vkDestroyInstance) \
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
    USE_VK_FUNC(vkGetPhysicalDeviceCooperativeMatrixFlexibleDimensionsPropertiesNV) \
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
    USE_VK_FUNC(vkGetPhysicalDeviceVideoCapabilitiesKHR) \
    USE_VK_FUNC(vkGetPhysicalDeviceVideoEncodeQualityLevelPropertiesKHR) \
    USE_VK_FUNC(vkGetPhysicalDeviceVideoFormatPropertiesKHR) \
    USE_VK_FUNC(vkGetPhysicalDeviceWin32PresentationSupportKHR)

#endif /* __WINE_VULKAN_THUNKS_H */
