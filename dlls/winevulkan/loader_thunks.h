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

#ifndef __WINE_VULKAN_LOADER_THUNKS_H
#define __WINE_VULKAN_LOADER_THUNKS_H

enum unix_call
{
    unix_init,
    unix_vkAcquireNextImage2KHR,
    unix_vkAcquireNextImageKHR,
    unix_vkAcquirePerformanceConfigurationINTEL,
    unix_vkAcquireProfilingLockKHR,
    unix_vkAllocateCommandBuffers,
    unix_vkAllocateDescriptorSets,
    unix_vkAllocateMemory,
    unix_vkBeginCommandBuffer,
    unix_vkBindAccelerationStructureMemoryNV,
    unix_vkBindBufferMemory,
    unix_vkBindBufferMemory2,
    unix_vkBindBufferMemory2KHR,
    unix_vkBindImageMemory,
    unix_vkBindImageMemory2,
    unix_vkBindImageMemory2KHR,
    unix_vkBuildAccelerationStructuresKHR,
    unix_vkCmdBeginConditionalRenderingEXT,
    unix_vkCmdBeginDebugUtilsLabelEXT,
    unix_vkCmdBeginQuery,
    unix_vkCmdBeginQueryIndexedEXT,
    unix_vkCmdBeginRenderPass,
    unix_vkCmdBeginRenderPass2,
    unix_vkCmdBeginRenderPass2KHR,
    unix_vkCmdBeginRendering,
    unix_vkCmdBeginRenderingKHR,
    unix_vkCmdBeginTransformFeedbackEXT,
    unix_vkCmdBindDescriptorSets,
    unix_vkCmdBindIndexBuffer,
    unix_vkCmdBindInvocationMaskHUAWEI,
    unix_vkCmdBindPipeline,
    unix_vkCmdBindPipelineShaderGroupNV,
    unix_vkCmdBindShadingRateImageNV,
    unix_vkCmdBindTransformFeedbackBuffersEXT,
    unix_vkCmdBindVertexBuffers,
    unix_vkCmdBindVertexBuffers2,
    unix_vkCmdBindVertexBuffers2EXT,
    unix_vkCmdBlitImage,
    unix_vkCmdBlitImage2,
    unix_vkCmdBlitImage2KHR,
    unix_vkCmdBuildAccelerationStructureNV,
    unix_vkCmdBuildAccelerationStructuresIndirectKHR,
    unix_vkCmdBuildAccelerationStructuresKHR,
    unix_vkCmdClearAttachments,
    unix_vkCmdClearColorImage,
    unix_vkCmdClearDepthStencilImage,
    unix_vkCmdCopyAccelerationStructureKHR,
    unix_vkCmdCopyAccelerationStructureNV,
    unix_vkCmdCopyAccelerationStructureToMemoryKHR,
    unix_vkCmdCopyBuffer,
    unix_vkCmdCopyBuffer2,
    unix_vkCmdCopyBuffer2KHR,
    unix_vkCmdCopyBufferToImage,
    unix_vkCmdCopyBufferToImage2,
    unix_vkCmdCopyBufferToImage2KHR,
    unix_vkCmdCopyImage,
    unix_vkCmdCopyImage2,
    unix_vkCmdCopyImage2KHR,
    unix_vkCmdCopyImageToBuffer,
    unix_vkCmdCopyImageToBuffer2,
    unix_vkCmdCopyImageToBuffer2KHR,
    unix_vkCmdCopyMemoryToAccelerationStructureKHR,
    unix_vkCmdCopyQueryPoolResults,
    unix_vkCmdCuLaunchKernelNVX,
    unix_vkCmdDebugMarkerBeginEXT,
    unix_vkCmdDebugMarkerEndEXT,
    unix_vkCmdDebugMarkerInsertEXT,
    unix_vkCmdDispatch,
    unix_vkCmdDispatchBase,
    unix_vkCmdDispatchBaseKHR,
    unix_vkCmdDispatchIndirect,
    unix_vkCmdDraw,
    unix_vkCmdDrawIndexed,
    unix_vkCmdDrawIndexedIndirect,
    unix_vkCmdDrawIndexedIndirectCount,
    unix_vkCmdDrawIndexedIndirectCountAMD,
    unix_vkCmdDrawIndexedIndirectCountKHR,
    unix_vkCmdDrawIndirect,
    unix_vkCmdDrawIndirectByteCountEXT,
    unix_vkCmdDrawIndirectCount,
    unix_vkCmdDrawIndirectCountAMD,
    unix_vkCmdDrawIndirectCountKHR,
    unix_vkCmdDrawMeshTasksIndirectCountNV,
    unix_vkCmdDrawMeshTasksIndirectNV,
    unix_vkCmdDrawMeshTasksNV,
    unix_vkCmdDrawMultiEXT,
    unix_vkCmdDrawMultiIndexedEXT,
    unix_vkCmdEndConditionalRenderingEXT,
    unix_vkCmdEndDebugUtilsLabelEXT,
    unix_vkCmdEndQuery,
    unix_vkCmdEndQueryIndexedEXT,
    unix_vkCmdEndRenderPass,
    unix_vkCmdEndRenderPass2,
    unix_vkCmdEndRenderPass2KHR,
    unix_vkCmdEndRendering,
    unix_vkCmdEndRenderingKHR,
    unix_vkCmdEndTransformFeedbackEXT,
    unix_vkCmdExecuteCommands,
    unix_vkCmdExecuteGeneratedCommandsNV,
    unix_vkCmdFillBuffer,
    unix_vkCmdInsertDebugUtilsLabelEXT,
    unix_vkCmdNextSubpass,
    unix_vkCmdNextSubpass2,
    unix_vkCmdNextSubpass2KHR,
    unix_vkCmdPipelineBarrier,
    unix_vkCmdPipelineBarrier2,
    unix_vkCmdPipelineBarrier2KHR,
    unix_vkCmdPreprocessGeneratedCommandsNV,
    unix_vkCmdPushConstants,
    unix_vkCmdPushDescriptorSetKHR,
    unix_vkCmdPushDescriptorSetWithTemplateKHR,
    unix_vkCmdResetEvent,
    unix_vkCmdResetEvent2,
    unix_vkCmdResetEvent2KHR,
    unix_vkCmdResetQueryPool,
    unix_vkCmdResolveImage,
    unix_vkCmdResolveImage2,
    unix_vkCmdResolveImage2KHR,
    unix_vkCmdSetBlendConstants,
    unix_vkCmdSetCheckpointNV,
    unix_vkCmdSetCoarseSampleOrderNV,
    unix_vkCmdSetColorWriteEnableEXT,
    unix_vkCmdSetCullMode,
    unix_vkCmdSetCullModeEXT,
    unix_vkCmdSetDepthBias,
    unix_vkCmdSetDepthBiasEnable,
    unix_vkCmdSetDepthBiasEnableEXT,
    unix_vkCmdSetDepthBounds,
    unix_vkCmdSetDepthBoundsTestEnable,
    unix_vkCmdSetDepthBoundsTestEnableEXT,
    unix_vkCmdSetDepthCompareOp,
    unix_vkCmdSetDepthCompareOpEXT,
    unix_vkCmdSetDepthTestEnable,
    unix_vkCmdSetDepthTestEnableEXT,
    unix_vkCmdSetDepthWriteEnable,
    unix_vkCmdSetDepthWriteEnableEXT,
    unix_vkCmdSetDeviceMask,
    unix_vkCmdSetDeviceMaskKHR,
    unix_vkCmdSetDiscardRectangleEXT,
    unix_vkCmdSetEvent,
    unix_vkCmdSetEvent2,
    unix_vkCmdSetEvent2KHR,
    unix_vkCmdSetExclusiveScissorNV,
    unix_vkCmdSetFragmentShadingRateEnumNV,
    unix_vkCmdSetFragmentShadingRateKHR,
    unix_vkCmdSetFrontFace,
    unix_vkCmdSetFrontFaceEXT,
    unix_vkCmdSetLineStippleEXT,
    unix_vkCmdSetLineWidth,
    unix_vkCmdSetLogicOpEXT,
    unix_vkCmdSetPatchControlPointsEXT,
    unix_vkCmdSetPerformanceMarkerINTEL,
    unix_vkCmdSetPerformanceOverrideINTEL,
    unix_vkCmdSetPerformanceStreamMarkerINTEL,
    unix_vkCmdSetPrimitiveRestartEnable,
    unix_vkCmdSetPrimitiveRestartEnableEXT,
    unix_vkCmdSetPrimitiveTopology,
    unix_vkCmdSetPrimitiveTopologyEXT,
    unix_vkCmdSetRasterizerDiscardEnable,
    unix_vkCmdSetRasterizerDiscardEnableEXT,
    unix_vkCmdSetRayTracingPipelineStackSizeKHR,
    unix_vkCmdSetSampleLocationsEXT,
    unix_vkCmdSetScissor,
    unix_vkCmdSetScissorWithCount,
    unix_vkCmdSetScissorWithCountEXT,
    unix_vkCmdSetStencilCompareMask,
    unix_vkCmdSetStencilOp,
    unix_vkCmdSetStencilOpEXT,
    unix_vkCmdSetStencilReference,
    unix_vkCmdSetStencilTestEnable,
    unix_vkCmdSetStencilTestEnableEXT,
    unix_vkCmdSetStencilWriteMask,
    unix_vkCmdSetVertexInputEXT,
    unix_vkCmdSetViewport,
    unix_vkCmdSetViewportShadingRatePaletteNV,
    unix_vkCmdSetViewportWScalingNV,
    unix_vkCmdSetViewportWithCount,
    unix_vkCmdSetViewportWithCountEXT,
    unix_vkCmdSubpassShadingHUAWEI,
    unix_vkCmdTraceRaysIndirect2KHR,
    unix_vkCmdTraceRaysIndirectKHR,
    unix_vkCmdTraceRaysKHR,
    unix_vkCmdTraceRaysNV,
    unix_vkCmdUpdateBuffer,
    unix_vkCmdWaitEvents,
    unix_vkCmdWaitEvents2,
    unix_vkCmdWaitEvents2KHR,
    unix_vkCmdWriteAccelerationStructuresPropertiesKHR,
    unix_vkCmdWriteAccelerationStructuresPropertiesNV,
    unix_vkCmdWriteBufferMarker2AMD,
    unix_vkCmdWriteBufferMarkerAMD,
    unix_vkCmdWriteTimestamp,
    unix_vkCmdWriteTimestamp2,
    unix_vkCmdWriteTimestamp2KHR,
    unix_vkCompileDeferredNV,
    unix_vkCopyAccelerationStructureKHR,
    unix_vkCopyAccelerationStructureToMemoryKHR,
    unix_vkCopyMemoryToAccelerationStructureKHR,
    unix_vkCreateAccelerationStructureKHR,
    unix_vkCreateAccelerationStructureNV,
    unix_vkCreateBuffer,
    unix_vkCreateBufferView,
    unix_vkCreateCommandPool,
    unix_vkCreateComputePipelines,
    unix_vkCreateCuFunctionNVX,
    unix_vkCreateCuModuleNVX,
    unix_vkCreateDebugReportCallbackEXT,
    unix_vkCreateDebugUtilsMessengerEXT,
    unix_vkCreateDeferredOperationKHR,
    unix_vkCreateDescriptorPool,
    unix_vkCreateDescriptorSetLayout,
    unix_vkCreateDescriptorUpdateTemplate,
    unix_vkCreateDescriptorUpdateTemplateKHR,
    unix_vkCreateDevice,
    unix_vkCreateEvent,
    unix_vkCreateFence,
    unix_vkCreateFramebuffer,
    unix_vkCreateGraphicsPipelines,
    unix_vkCreateImage,
    unix_vkCreateImageView,
    unix_vkCreateIndirectCommandsLayoutNV,
    unix_vkCreateInstance,
    unix_vkCreatePipelineCache,
    unix_vkCreatePipelineLayout,
    unix_vkCreatePrivateDataSlot,
    unix_vkCreatePrivateDataSlotEXT,
    unix_vkCreateQueryPool,
    unix_vkCreateRayTracingPipelinesKHR,
    unix_vkCreateRayTracingPipelinesNV,
    unix_vkCreateRenderPass,
    unix_vkCreateRenderPass2,
    unix_vkCreateRenderPass2KHR,
    unix_vkCreateSampler,
    unix_vkCreateSamplerYcbcrConversion,
    unix_vkCreateSamplerYcbcrConversionKHR,
    unix_vkCreateSemaphore,
    unix_vkCreateShaderModule,
    unix_vkCreateSwapchainKHR,
    unix_vkCreateValidationCacheEXT,
    unix_vkCreateWin32SurfaceKHR,
    unix_vkDebugMarkerSetObjectNameEXT,
    unix_vkDebugMarkerSetObjectTagEXT,
    unix_vkDebugReportMessageEXT,
    unix_vkDeferredOperationJoinKHR,
    unix_vkDestroyAccelerationStructureKHR,
    unix_vkDestroyAccelerationStructureNV,
    unix_vkDestroyBuffer,
    unix_vkDestroyBufferView,
    unix_vkDestroyCommandPool,
    unix_vkDestroyCuFunctionNVX,
    unix_vkDestroyCuModuleNVX,
    unix_vkDestroyDebugReportCallbackEXT,
    unix_vkDestroyDebugUtilsMessengerEXT,
    unix_vkDestroyDeferredOperationKHR,
    unix_vkDestroyDescriptorPool,
    unix_vkDestroyDescriptorSetLayout,
    unix_vkDestroyDescriptorUpdateTemplate,
    unix_vkDestroyDescriptorUpdateTemplateKHR,
    unix_vkDestroyDevice,
    unix_vkDestroyEvent,
    unix_vkDestroyFence,
    unix_vkDestroyFramebuffer,
    unix_vkDestroyImage,
    unix_vkDestroyImageView,
    unix_vkDestroyIndirectCommandsLayoutNV,
    unix_vkDestroyInstance,
    unix_vkDestroyPipeline,
    unix_vkDestroyPipelineCache,
    unix_vkDestroyPipelineLayout,
    unix_vkDestroyPrivateDataSlot,
    unix_vkDestroyPrivateDataSlotEXT,
    unix_vkDestroyQueryPool,
    unix_vkDestroyRenderPass,
    unix_vkDestroySampler,
    unix_vkDestroySamplerYcbcrConversion,
    unix_vkDestroySamplerYcbcrConversionKHR,
    unix_vkDestroySemaphore,
    unix_vkDestroyShaderModule,
    unix_vkDestroySurfaceKHR,
    unix_vkDestroySwapchainKHR,
    unix_vkDestroyValidationCacheEXT,
    unix_vkDeviceWaitIdle,
    unix_vkEndCommandBuffer,
    unix_vkEnumerateDeviceExtensionProperties,
    unix_vkEnumerateDeviceLayerProperties,
    unix_vkEnumerateInstanceExtensionProperties,
    unix_vkEnumerateInstanceVersion,
    unix_vkEnumeratePhysicalDeviceGroups,
    unix_vkEnumeratePhysicalDeviceGroupsKHR,
    unix_vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR,
    unix_vkEnumeratePhysicalDevices,
    unix_vkFlushMappedMemoryRanges,
    unix_vkFreeCommandBuffers,
    unix_vkFreeDescriptorSets,
    unix_vkFreeMemory,
    unix_vkGetAccelerationStructureBuildSizesKHR,
    unix_vkGetAccelerationStructureDeviceAddressKHR,
    unix_vkGetAccelerationStructureHandleNV,
    unix_vkGetAccelerationStructureMemoryRequirementsNV,
    unix_vkGetBufferDeviceAddress,
    unix_vkGetBufferDeviceAddressEXT,
    unix_vkGetBufferDeviceAddressKHR,
    unix_vkGetBufferMemoryRequirements,
    unix_vkGetBufferMemoryRequirements2,
    unix_vkGetBufferMemoryRequirements2KHR,
    unix_vkGetBufferOpaqueCaptureAddress,
    unix_vkGetBufferOpaqueCaptureAddressKHR,
    unix_vkGetCalibratedTimestampsEXT,
    unix_vkGetDeferredOperationMaxConcurrencyKHR,
    unix_vkGetDeferredOperationResultKHR,
    unix_vkGetDescriptorSetHostMappingVALVE,
    unix_vkGetDescriptorSetLayoutHostMappingInfoVALVE,
    unix_vkGetDescriptorSetLayoutSupport,
    unix_vkGetDescriptorSetLayoutSupportKHR,
    unix_vkGetDeviceAccelerationStructureCompatibilityKHR,
    unix_vkGetDeviceBufferMemoryRequirements,
    unix_vkGetDeviceBufferMemoryRequirementsKHR,
    unix_vkGetDeviceGroupPeerMemoryFeatures,
    unix_vkGetDeviceGroupPeerMemoryFeaturesKHR,
    unix_vkGetDeviceGroupPresentCapabilitiesKHR,
    unix_vkGetDeviceGroupSurfacePresentModesKHR,
    unix_vkGetDeviceImageMemoryRequirements,
    unix_vkGetDeviceImageMemoryRequirementsKHR,
    unix_vkGetDeviceImageSparseMemoryRequirements,
    unix_vkGetDeviceImageSparseMemoryRequirementsKHR,
    unix_vkGetDeviceMemoryCommitment,
    unix_vkGetDeviceMemoryOpaqueCaptureAddress,
    unix_vkGetDeviceMemoryOpaqueCaptureAddressKHR,
    unix_vkGetDeviceQueue,
    unix_vkGetDeviceQueue2,
    unix_vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI,
    unix_vkGetDynamicRenderingTilePropertiesQCOM,
    unix_vkGetEventStatus,
    unix_vkGetFenceStatus,
    unix_vkGetFramebufferTilePropertiesQCOM,
    unix_vkGetGeneratedCommandsMemoryRequirementsNV,
    unix_vkGetImageMemoryRequirements,
    unix_vkGetImageMemoryRequirements2,
    unix_vkGetImageMemoryRequirements2KHR,
    unix_vkGetImageSparseMemoryRequirements,
    unix_vkGetImageSparseMemoryRequirements2,
    unix_vkGetImageSparseMemoryRequirements2KHR,
    unix_vkGetImageSubresourceLayout,
    unix_vkGetImageSubresourceLayout2EXT,
    unix_vkGetImageViewAddressNVX,
    unix_vkGetImageViewHandleNVX,
    unix_vkGetMemoryHostPointerPropertiesEXT,
    unix_vkGetPerformanceParameterINTEL,
    unix_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT,
    unix_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV,
    unix_vkGetPhysicalDeviceExternalBufferProperties,
    unix_vkGetPhysicalDeviceExternalBufferPropertiesKHR,
    unix_vkGetPhysicalDeviceExternalFenceProperties,
    unix_vkGetPhysicalDeviceExternalFencePropertiesKHR,
    unix_vkGetPhysicalDeviceExternalSemaphoreProperties,
    unix_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR,
    unix_vkGetPhysicalDeviceFeatures,
    unix_vkGetPhysicalDeviceFeatures2,
    unix_vkGetPhysicalDeviceFeatures2KHR,
    unix_vkGetPhysicalDeviceFormatProperties,
    unix_vkGetPhysicalDeviceFormatProperties2,
    unix_vkGetPhysicalDeviceFormatProperties2KHR,
    unix_vkGetPhysicalDeviceFragmentShadingRatesKHR,
    unix_vkGetPhysicalDeviceImageFormatProperties,
    unix_vkGetPhysicalDeviceImageFormatProperties2,
    unix_vkGetPhysicalDeviceImageFormatProperties2KHR,
    unix_vkGetPhysicalDeviceMemoryProperties,
    unix_vkGetPhysicalDeviceMemoryProperties2,
    unix_vkGetPhysicalDeviceMemoryProperties2KHR,
    unix_vkGetPhysicalDeviceMultisamplePropertiesEXT,
    unix_vkGetPhysicalDevicePresentRectanglesKHR,
    unix_vkGetPhysicalDeviceProperties,
    unix_vkGetPhysicalDeviceProperties2,
    unix_vkGetPhysicalDeviceProperties2KHR,
    unix_vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR,
    unix_vkGetPhysicalDeviceQueueFamilyProperties,
    unix_vkGetPhysicalDeviceQueueFamilyProperties2,
    unix_vkGetPhysicalDeviceQueueFamilyProperties2KHR,
    unix_vkGetPhysicalDeviceSparseImageFormatProperties,
    unix_vkGetPhysicalDeviceSparseImageFormatProperties2,
    unix_vkGetPhysicalDeviceSparseImageFormatProperties2KHR,
    unix_vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV,
    unix_vkGetPhysicalDeviceSurfaceCapabilities2KHR,
    unix_vkGetPhysicalDeviceSurfaceCapabilitiesKHR,
    unix_vkGetPhysicalDeviceSurfaceFormats2KHR,
    unix_vkGetPhysicalDeviceSurfaceFormatsKHR,
    unix_vkGetPhysicalDeviceSurfacePresentModesKHR,
    unix_vkGetPhysicalDeviceSurfaceSupportKHR,
    unix_vkGetPhysicalDeviceToolProperties,
    unix_vkGetPhysicalDeviceToolPropertiesEXT,
    unix_vkGetPhysicalDeviceWin32PresentationSupportKHR,
    unix_vkGetPipelineCacheData,
    unix_vkGetPipelineExecutableInternalRepresentationsKHR,
    unix_vkGetPipelineExecutablePropertiesKHR,
    unix_vkGetPipelineExecutableStatisticsKHR,
    unix_vkGetPipelinePropertiesEXT,
    unix_vkGetPrivateData,
    unix_vkGetPrivateDataEXT,
    unix_vkGetQueryPoolResults,
    unix_vkGetQueueCheckpointData2NV,
    unix_vkGetQueueCheckpointDataNV,
    unix_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR,
    unix_vkGetRayTracingShaderGroupHandlesKHR,
    unix_vkGetRayTracingShaderGroupHandlesNV,
    unix_vkGetRayTracingShaderGroupStackSizeKHR,
    unix_vkGetRenderAreaGranularity,
    unix_vkGetSemaphoreCounterValue,
    unix_vkGetSemaphoreCounterValueKHR,
    unix_vkGetShaderInfoAMD,
    unix_vkGetShaderModuleCreateInfoIdentifierEXT,
    unix_vkGetShaderModuleIdentifierEXT,
    unix_vkGetSwapchainImagesKHR,
    unix_vkGetValidationCacheDataEXT,
    unix_vkInitializePerformanceApiINTEL,
    unix_vkInvalidateMappedMemoryRanges,
    unix_vkMapMemory,
    unix_vkMergePipelineCaches,
    unix_vkMergeValidationCachesEXT,
    unix_vkQueueBeginDebugUtilsLabelEXT,
    unix_vkQueueBindSparse,
    unix_vkQueueEndDebugUtilsLabelEXT,
    unix_vkQueueInsertDebugUtilsLabelEXT,
    unix_vkQueuePresentKHR,
    unix_vkQueueSetPerformanceConfigurationINTEL,
    unix_vkQueueSubmit,
    unix_vkQueueSubmit2,
    unix_vkQueueSubmit2KHR,
    unix_vkQueueWaitIdle,
    unix_vkReleasePerformanceConfigurationINTEL,
    unix_vkReleaseProfilingLockKHR,
    unix_vkResetCommandBuffer,
    unix_vkResetCommandPool,
    unix_vkResetDescriptorPool,
    unix_vkResetEvent,
    unix_vkResetFences,
    unix_vkResetQueryPool,
    unix_vkResetQueryPoolEXT,
    unix_vkSetDebugUtilsObjectNameEXT,
    unix_vkSetDebugUtilsObjectTagEXT,
    unix_vkSetDeviceMemoryPriorityEXT,
    unix_vkSetEvent,
    unix_vkSetPrivateData,
    unix_vkSetPrivateDataEXT,
    unix_vkSignalSemaphore,
    unix_vkSignalSemaphoreKHR,
    unix_vkSubmitDebugUtilsMessageEXT,
    unix_vkTrimCommandPool,
    unix_vkTrimCommandPoolKHR,
    unix_vkUninitializePerformanceApiINTEL,
    unix_vkUnmapMemory,
    unix_vkUpdateDescriptorSetWithTemplate,
    unix_vkUpdateDescriptorSetWithTemplateKHR,
    unix_vkUpdateDescriptorSets,
    unix_vkWaitForFences,
    unix_vkWaitForPresentKHR,
    unix_vkWaitSemaphores,
    unix_vkWaitSemaphoresKHR,
    unix_vkWriteAccelerationStructuresPropertiesKHR,
    unix_count,
};

#include "pshpack4.h"

struct vkAcquireNextImage2KHR_params
{
    VkDevice device;
    const VkAcquireNextImageInfoKHR *pAcquireInfo;
    uint32_t *pImageIndex;
};

struct vkAcquireNextImageKHR_params
{
    VkDevice device;
    VkSwapchainKHR swapchain;
    uint64_t timeout;
    VkSemaphore semaphore;
    VkFence fence;
    uint32_t *pImageIndex;
};

struct vkAcquirePerformanceConfigurationINTEL_params
{
    VkDevice device;
    const VkPerformanceConfigurationAcquireInfoINTEL *pAcquireInfo;
    VkPerformanceConfigurationINTEL *pConfiguration;
};

struct vkAcquireProfilingLockKHR_params
{
    VkDevice device;
    const VkAcquireProfilingLockInfoKHR *pInfo;
};

struct vkAllocateCommandBuffers_params
{
    VkDevice device;
    const VkCommandBufferAllocateInfo *pAllocateInfo;
    VkCommandBuffer *pCommandBuffers;
};

struct vkAllocateDescriptorSets_params
{
    VkDevice device;
    const VkDescriptorSetAllocateInfo *pAllocateInfo;
    VkDescriptorSet *pDescriptorSets;
};

struct vkAllocateMemory_params
{
    VkDevice device;
    const VkMemoryAllocateInfo *pAllocateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkDeviceMemory *pMemory;
};

struct vkBeginCommandBuffer_params
{
    VkCommandBuffer commandBuffer;
    const VkCommandBufferBeginInfo *pBeginInfo;
};

struct vkBindAccelerationStructureMemoryNV_params
{
    VkDevice device;
    uint32_t bindInfoCount;
    const VkBindAccelerationStructureMemoryInfoNV *pBindInfos;
};

struct vkBindBufferMemory_params
{
    VkDevice device;
    VkBuffer buffer;
    VkDeviceMemory memory;
    VkDeviceSize memoryOffset;
};

struct vkBindBufferMemory2_params
{
    VkDevice device;
    uint32_t bindInfoCount;
    const VkBindBufferMemoryInfo *pBindInfos;
};

struct vkBindBufferMemory2KHR_params
{
    VkDevice device;
    uint32_t bindInfoCount;
    const VkBindBufferMemoryInfo *pBindInfos;
};

struct vkBindImageMemory_params
{
    VkDevice device;
    VkImage image;
    VkDeviceMemory memory;
    VkDeviceSize memoryOffset;
};

struct vkBindImageMemory2_params
{
    VkDevice device;
    uint32_t bindInfoCount;
    const VkBindImageMemoryInfo *pBindInfos;
};

struct vkBindImageMemory2KHR_params
{
    VkDevice device;
    uint32_t bindInfoCount;
    const VkBindImageMemoryInfo *pBindInfos;
};

struct vkBuildAccelerationStructuresKHR_params
{
    VkDevice device;
    VkDeferredOperationKHR deferredOperation;
    uint32_t infoCount;
    const VkAccelerationStructureBuildGeometryInfoKHR *pInfos;
    const VkAccelerationStructureBuildRangeInfoKHR * const*ppBuildRangeInfos;
};

struct vkCmdBeginConditionalRenderingEXT_params
{
    VkCommandBuffer commandBuffer;
    const VkConditionalRenderingBeginInfoEXT *pConditionalRenderingBegin;
};

struct vkCmdBeginDebugUtilsLabelEXT_params
{
    VkCommandBuffer commandBuffer;
    const VkDebugUtilsLabelEXT *pLabelInfo;
};

struct vkCmdBeginQuery_params
{
    VkCommandBuffer commandBuffer;
    VkQueryPool queryPool;
    uint32_t query;
    VkQueryControlFlags flags;
};

struct vkCmdBeginQueryIndexedEXT_params
{
    VkCommandBuffer commandBuffer;
    VkQueryPool queryPool;
    uint32_t query;
    VkQueryControlFlags flags;
    uint32_t index;
};

struct vkCmdBeginRenderPass_params
{
    VkCommandBuffer commandBuffer;
    const VkRenderPassBeginInfo *pRenderPassBegin;
    VkSubpassContents contents;
};

struct vkCmdBeginRenderPass2_params
{
    VkCommandBuffer commandBuffer;
    const VkRenderPassBeginInfo *pRenderPassBegin;
    const VkSubpassBeginInfo *pSubpassBeginInfo;
};

struct vkCmdBeginRenderPass2KHR_params
{
    VkCommandBuffer commandBuffer;
    const VkRenderPassBeginInfo *pRenderPassBegin;
    const VkSubpassBeginInfo *pSubpassBeginInfo;
};

struct vkCmdBeginRendering_params
{
    VkCommandBuffer commandBuffer;
    const VkRenderingInfo *pRenderingInfo;
};

struct vkCmdBeginRenderingKHR_params
{
    VkCommandBuffer commandBuffer;
    const VkRenderingInfo *pRenderingInfo;
};

struct vkCmdBeginTransformFeedbackEXT_params
{
    VkCommandBuffer commandBuffer;
    uint32_t firstCounterBuffer;
    uint32_t counterBufferCount;
    const VkBuffer *pCounterBuffers;
    const VkDeviceSize *pCounterBufferOffsets;
};

struct vkCmdBindDescriptorSets_params
{
    VkCommandBuffer commandBuffer;
    VkPipelineBindPoint pipelineBindPoint;
    VkPipelineLayout layout;
    uint32_t firstSet;
    uint32_t descriptorSetCount;
    const VkDescriptorSet *pDescriptorSets;
    uint32_t dynamicOffsetCount;
    const uint32_t *pDynamicOffsets;
};

struct vkCmdBindIndexBuffer_params
{
    VkCommandBuffer commandBuffer;
    VkBuffer buffer;
    VkDeviceSize offset;
    VkIndexType indexType;
};

struct vkCmdBindInvocationMaskHUAWEI_params
{
    VkCommandBuffer commandBuffer;
    VkImageView imageView;
    VkImageLayout imageLayout;
};

struct vkCmdBindPipeline_params
{
    VkCommandBuffer commandBuffer;
    VkPipelineBindPoint pipelineBindPoint;
    VkPipeline pipeline;
};

struct vkCmdBindPipelineShaderGroupNV_params
{
    VkCommandBuffer commandBuffer;
    VkPipelineBindPoint pipelineBindPoint;
    VkPipeline pipeline;
    uint32_t groupIndex;
};

struct vkCmdBindShadingRateImageNV_params
{
    VkCommandBuffer commandBuffer;
    VkImageView imageView;
    VkImageLayout imageLayout;
};

struct vkCmdBindTransformFeedbackBuffersEXT_params
{
    VkCommandBuffer commandBuffer;
    uint32_t firstBinding;
    uint32_t bindingCount;
    const VkBuffer *pBuffers;
    const VkDeviceSize *pOffsets;
    const VkDeviceSize *pSizes;
};

struct vkCmdBindVertexBuffers_params
{
    VkCommandBuffer commandBuffer;
    uint32_t firstBinding;
    uint32_t bindingCount;
    const VkBuffer *pBuffers;
    const VkDeviceSize *pOffsets;
};

struct vkCmdBindVertexBuffers2_params
{
    VkCommandBuffer commandBuffer;
    uint32_t firstBinding;
    uint32_t bindingCount;
    const VkBuffer *pBuffers;
    const VkDeviceSize *pOffsets;
    const VkDeviceSize *pSizes;
    const VkDeviceSize *pStrides;
};

struct vkCmdBindVertexBuffers2EXT_params
{
    VkCommandBuffer commandBuffer;
    uint32_t firstBinding;
    uint32_t bindingCount;
    const VkBuffer *pBuffers;
    const VkDeviceSize *pOffsets;
    const VkDeviceSize *pSizes;
    const VkDeviceSize *pStrides;
};

struct vkCmdBlitImage_params
{
    VkCommandBuffer commandBuffer;
    VkImage srcImage;
    VkImageLayout srcImageLayout;
    VkImage dstImage;
    VkImageLayout dstImageLayout;
    uint32_t regionCount;
    const VkImageBlit *pRegions;
    VkFilter filter;
};

struct vkCmdBlitImage2_params
{
    VkCommandBuffer commandBuffer;
    const VkBlitImageInfo2 *pBlitImageInfo;
};

struct vkCmdBlitImage2KHR_params
{
    VkCommandBuffer commandBuffer;
    const VkBlitImageInfo2 *pBlitImageInfo;
};

struct vkCmdBuildAccelerationStructureNV_params
{
    VkCommandBuffer commandBuffer;
    const VkAccelerationStructureInfoNV *pInfo;
    VkBuffer instanceData;
    VkDeviceSize instanceOffset;
    VkBool32 update;
    VkAccelerationStructureNV dst;
    VkAccelerationStructureNV src;
    VkBuffer scratch;
    VkDeviceSize scratchOffset;
};

struct vkCmdBuildAccelerationStructuresIndirectKHR_params
{
    VkCommandBuffer commandBuffer;
    uint32_t infoCount;
    const VkAccelerationStructureBuildGeometryInfoKHR *pInfos;
    const VkDeviceAddress *pIndirectDeviceAddresses;
    const uint32_t *pIndirectStrides;
    const uint32_t * const*ppMaxPrimitiveCounts;
};

struct vkCmdBuildAccelerationStructuresKHR_params
{
    VkCommandBuffer commandBuffer;
    uint32_t infoCount;
    const VkAccelerationStructureBuildGeometryInfoKHR *pInfos;
    const VkAccelerationStructureBuildRangeInfoKHR * const*ppBuildRangeInfos;
};

struct vkCmdClearAttachments_params
{
    VkCommandBuffer commandBuffer;
    uint32_t attachmentCount;
    const VkClearAttachment *pAttachments;
    uint32_t rectCount;
    const VkClearRect *pRects;
};

struct vkCmdClearColorImage_params
{
    VkCommandBuffer commandBuffer;
    VkImage image;
    VkImageLayout imageLayout;
    const VkClearColorValue *pColor;
    uint32_t rangeCount;
    const VkImageSubresourceRange *pRanges;
};

struct vkCmdClearDepthStencilImage_params
{
    VkCommandBuffer commandBuffer;
    VkImage image;
    VkImageLayout imageLayout;
    const VkClearDepthStencilValue *pDepthStencil;
    uint32_t rangeCount;
    const VkImageSubresourceRange *pRanges;
};

struct vkCmdCopyAccelerationStructureKHR_params
{
    VkCommandBuffer commandBuffer;
    const VkCopyAccelerationStructureInfoKHR *pInfo;
};

struct vkCmdCopyAccelerationStructureNV_params
{
    VkCommandBuffer commandBuffer;
    VkAccelerationStructureNV dst;
    VkAccelerationStructureNV src;
    VkCopyAccelerationStructureModeKHR mode;
};

struct vkCmdCopyAccelerationStructureToMemoryKHR_params
{
    VkCommandBuffer commandBuffer;
    const VkCopyAccelerationStructureToMemoryInfoKHR *pInfo;
};

struct vkCmdCopyBuffer_params
{
    VkCommandBuffer commandBuffer;
    VkBuffer srcBuffer;
    VkBuffer dstBuffer;
    uint32_t regionCount;
    const VkBufferCopy *pRegions;
};

struct vkCmdCopyBuffer2_params
{
    VkCommandBuffer commandBuffer;
    const VkCopyBufferInfo2 *pCopyBufferInfo;
};

struct vkCmdCopyBuffer2KHR_params
{
    VkCommandBuffer commandBuffer;
    const VkCopyBufferInfo2 *pCopyBufferInfo;
};

struct vkCmdCopyBufferToImage_params
{
    VkCommandBuffer commandBuffer;
    VkBuffer srcBuffer;
    VkImage dstImage;
    VkImageLayout dstImageLayout;
    uint32_t regionCount;
    const VkBufferImageCopy *pRegions;
};

struct vkCmdCopyBufferToImage2_params
{
    VkCommandBuffer commandBuffer;
    const VkCopyBufferToImageInfo2 *pCopyBufferToImageInfo;
};

struct vkCmdCopyBufferToImage2KHR_params
{
    VkCommandBuffer commandBuffer;
    const VkCopyBufferToImageInfo2 *pCopyBufferToImageInfo;
};

struct vkCmdCopyImage_params
{
    VkCommandBuffer commandBuffer;
    VkImage srcImage;
    VkImageLayout srcImageLayout;
    VkImage dstImage;
    VkImageLayout dstImageLayout;
    uint32_t regionCount;
    const VkImageCopy *pRegions;
};

struct vkCmdCopyImage2_params
{
    VkCommandBuffer commandBuffer;
    const VkCopyImageInfo2 *pCopyImageInfo;
};

struct vkCmdCopyImage2KHR_params
{
    VkCommandBuffer commandBuffer;
    const VkCopyImageInfo2 *pCopyImageInfo;
};

struct vkCmdCopyImageToBuffer_params
{
    VkCommandBuffer commandBuffer;
    VkImage srcImage;
    VkImageLayout srcImageLayout;
    VkBuffer dstBuffer;
    uint32_t regionCount;
    const VkBufferImageCopy *pRegions;
};

struct vkCmdCopyImageToBuffer2_params
{
    VkCommandBuffer commandBuffer;
    const VkCopyImageToBufferInfo2 *pCopyImageToBufferInfo;
};

struct vkCmdCopyImageToBuffer2KHR_params
{
    VkCommandBuffer commandBuffer;
    const VkCopyImageToBufferInfo2 *pCopyImageToBufferInfo;
};

struct vkCmdCopyMemoryToAccelerationStructureKHR_params
{
    VkCommandBuffer commandBuffer;
    const VkCopyMemoryToAccelerationStructureInfoKHR *pInfo;
};

struct vkCmdCopyQueryPoolResults_params
{
    VkCommandBuffer commandBuffer;
    VkQueryPool queryPool;
    uint32_t firstQuery;
    uint32_t queryCount;
    VkBuffer dstBuffer;
    VkDeviceSize dstOffset;
    VkDeviceSize stride;
    VkQueryResultFlags flags;
};

struct vkCmdCuLaunchKernelNVX_params
{
    VkCommandBuffer commandBuffer;
    const VkCuLaunchInfoNVX *pLaunchInfo;
};

struct vkCmdDebugMarkerBeginEXT_params
{
    VkCommandBuffer commandBuffer;
    const VkDebugMarkerMarkerInfoEXT *pMarkerInfo;
};

struct vkCmdDebugMarkerEndEXT_params
{
    VkCommandBuffer commandBuffer;
};

struct vkCmdDebugMarkerInsertEXT_params
{
    VkCommandBuffer commandBuffer;
    const VkDebugMarkerMarkerInfoEXT *pMarkerInfo;
};

struct vkCmdDispatch_params
{
    VkCommandBuffer commandBuffer;
    uint32_t groupCountX;
    uint32_t groupCountY;
    uint32_t groupCountZ;
};

struct vkCmdDispatchBase_params
{
    VkCommandBuffer commandBuffer;
    uint32_t baseGroupX;
    uint32_t baseGroupY;
    uint32_t baseGroupZ;
    uint32_t groupCountX;
    uint32_t groupCountY;
    uint32_t groupCountZ;
};

struct vkCmdDispatchBaseKHR_params
{
    VkCommandBuffer commandBuffer;
    uint32_t baseGroupX;
    uint32_t baseGroupY;
    uint32_t baseGroupZ;
    uint32_t groupCountX;
    uint32_t groupCountY;
    uint32_t groupCountZ;
};

struct vkCmdDispatchIndirect_params
{
    VkCommandBuffer commandBuffer;
    VkBuffer buffer;
    VkDeviceSize offset;
};

struct vkCmdDraw_params
{
    VkCommandBuffer commandBuffer;
    uint32_t vertexCount;
    uint32_t instanceCount;
    uint32_t firstVertex;
    uint32_t firstInstance;
};

struct vkCmdDrawIndexed_params
{
    VkCommandBuffer commandBuffer;
    uint32_t indexCount;
    uint32_t instanceCount;
    uint32_t firstIndex;
    int32_t vertexOffset;
    uint32_t firstInstance;
};

struct vkCmdDrawIndexedIndirect_params
{
    VkCommandBuffer commandBuffer;
    VkBuffer buffer;
    VkDeviceSize offset;
    uint32_t drawCount;
    uint32_t stride;
};

struct vkCmdDrawIndexedIndirectCount_params
{
    VkCommandBuffer commandBuffer;
    VkBuffer buffer;
    VkDeviceSize offset;
    VkBuffer countBuffer;
    VkDeviceSize countBufferOffset;
    uint32_t maxDrawCount;
    uint32_t stride;
};

struct vkCmdDrawIndexedIndirectCountAMD_params
{
    VkCommandBuffer commandBuffer;
    VkBuffer buffer;
    VkDeviceSize offset;
    VkBuffer countBuffer;
    VkDeviceSize countBufferOffset;
    uint32_t maxDrawCount;
    uint32_t stride;
};

struct vkCmdDrawIndexedIndirectCountKHR_params
{
    VkCommandBuffer commandBuffer;
    VkBuffer buffer;
    VkDeviceSize offset;
    VkBuffer countBuffer;
    VkDeviceSize countBufferOffset;
    uint32_t maxDrawCount;
    uint32_t stride;
};

struct vkCmdDrawIndirect_params
{
    VkCommandBuffer commandBuffer;
    VkBuffer buffer;
    VkDeviceSize offset;
    uint32_t drawCount;
    uint32_t stride;
};

struct vkCmdDrawIndirectByteCountEXT_params
{
    VkCommandBuffer commandBuffer;
    uint32_t instanceCount;
    uint32_t firstInstance;
    VkBuffer counterBuffer;
    VkDeviceSize counterBufferOffset;
    uint32_t counterOffset;
    uint32_t vertexStride;
};

struct vkCmdDrawIndirectCount_params
{
    VkCommandBuffer commandBuffer;
    VkBuffer buffer;
    VkDeviceSize offset;
    VkBuffer countBuffer;
    VkDeviceSize countBufferOffset;
    uint32_t maxDrawCount;
    uint32_t stride;
};

struct vkCmdDrawIndirectCountAMD_params
{
    VkCommandBuffer commandBuffer;
    VkBuffer buffer;
    VkDeviceSize offset;
    VkBuffer countBuffer;
    VkDeviceSize countBufferOffset;
    uint32_t maxDrawCount;
    uint32_t stride;
};

struct vkCmdDrawIndirectCountKHR_params
{
    VkCommandBuffer commandBuffer;
    VkBuffer buffer;
    VkDeviceSize offset;
    VkBuffer countBuffer;
    VkDeviceSize countBufferOffset;
    uint32_t maxDrawCount;
    uint32_t stride;
};

struct vkCmdDrawMeshTasksIndirectCountNV_params
{
    VkCommandBuffer commandBuffer;
    VkBuffer buffer;
    VkDeviceSize offset;
    VkBuffer countBuffer;
    VkDeviceSize countBufferOffset;
    uint32_t maxDrawCount;
    uint32_t stride;
};

struct vkCmdDrawMeshTasksIndirectNV_params
{
    VkCommandBuffer commandBuffer;
    VkBuffer buffer;
    VkDeviceSize offset;
    uint32_t drawCount;
    uint32_t stride;
};

struct vkCmdDrawMeshTasksNV_params
{
    VkCommandBuffer commandBuffer;
    uint32_t taskCount;
    uint32_t firstTask;
};

struct vkCmdDrawMultiEXT_params
{
    VkCommandBuffer commandBuffer;
    uint32_t drawCount;
    const VkMultiDrawInfoEXT *pVertexInfo;
    uint32_t instanceCount;
    uint32_t firstInstance;
    uint32_t stride;
};

struct vkCmdDrawMultiIndexedEXT_params
{
    VkCommandBuffer commandBuffer;
    uint32_t drawCount;
    const VkMultiDrawIndexedInfoEXT *pIndexInfo;
    uint32_t instanceCount;
    uint32_t firstInstance;
    uint32_t stride;
    const int32_t *pVertexOffset;
};

struct vkCmdEndConditionalRenderingEXT_params
{
    VkCommandBuffer commandBuffer;
};

struct vkCmdEndDebugUtilsLabelEXT_params
{
    VkCommandBuffer commandBuffer;
};

struct vkCmdEndQuery_params
{
    VkCommandBuffer commandBuffer;
    VkQueryPool queryPool;
    uint32_t query;
};

struct vkCmdEndQueryIndexedEXT_params
{
    VkCommandBuffer commandBuffer;
    VkQueryPool queryPool;
    uint32_t query;
    uint32_t index;
};

struct vkCmdEndRenderPass_params
{
    VkCommandBuffer commandBuffer;
};

struct vkCmdEndRenderPass2_params
{
    VkCommandBuffer commandBuffer;
    const VkSubpassEndInfo *pSubpassEndInfo;
};

struct vkCmdEndRenderPass2KHR_params
{
    VkCommandBuffer commandBuffer;
    const VkSubpassEndInfo *pSubpassEndInfo;
};

struct vkCmdEndRendering_params
{
    VkCommandBuffer commandBuffer;
};

struct vkCmdEndRenderingKHR_params
{
    VkCommandBuffer commandBuffer;
};

struct vkCmdEndTransformFeedbackEXT_params
{
    VkCommandBuffer commandBuffer;
    uint32_t firstCounterBuffer;
    uint32_t counterBufferCount;
    const VkBuffer *pCounterBuffers;
    const VkDeviceSize *pCounterBufferOffsets;
};

struct vkCmdExecuteCommands_params
{
    VkCommandBuffer commandBuffer;
    uint32_t commandBufferCount;
    const VkCommandBuffer *pCommandBuffers;
};

struct vkCmdExecuteGeneratedCommandsNV_params
{
    VkCommandBuffer commandBuffer;
    VkBool32 isPreprocessed;
    const VkGeneratedCommandsInfoNV *pGeneratedCommandsInfo;
};

struct vkCmdFillBuffer_params
{
    VkCommandBuffer commandBuffer;
    VkBuffer dstBuffer;
    VkDeviceSize dstOffset;
    VkDeviceSize size;
    uint32_t data;
};

struct vkCmdInsertDebugUtilsLabelEXT_params
{
    VkCommandBuffer commandBuffer;
    const VkDebugUtilsLabelEXT *pLabelInfo;
};

struct vkCmdNextSubpass_params
{
    VkCommandBuffer commandBuffer;
    VkSubpassContents contents;
};

struct vkCmdNextSubpass2_params
{
    VkCommandBuffer commandBuffer;
    const VkSubpassBeginInfo *pSubpassBeginInfo;
    const VkSubpassEndInfo *pSubpassEndInfo;
};

struct vkCmdNextSubpass2KHR_params
{
    VkCommandBuffer commandBuffer;
    const VkSubpassBeginInfo *pSubpassBeginInfo;
    const VkSubpassEndInfo *pSubpassEndInfo;
};

struct vkCmdPipelineBarrier_params
{
    VkCommandBuffer commandBuffer;
    VkPipelineStageFlags srcStageMask;
    VkPipelineStageFlags dstStageMask;
    VkDependencyFlags dependencyFlags;
    uint32_t memoryBarrierCount;
    const VkMemoryBarrier *pMemoryBarriers;
    uint32_t bufferMemoryBarrierCount;
    const VkBufferMemoryBarrier *pBufferMemoryBarriers;
    uint32_t imageMemoryBarrierCount;
    const VkImageMemoryBarrier *pImageMemoryBarriers;
};

struct vkCmdPipelineBarrier2_params
{
    VkCommandBuffer commandBuffer;
    const VkDependencyInfo *pDependencyInfo;
};

struct vkCmdPipelineBarrier2KHR_params
{
    VkCommandBuffer commandBuffer;
    const VkDependencyInfo *pDependencyInfo;
};

struct vkCmdPreprocessGeneratedCommandsNV_params
{
    VkCommandBuffer commandBuffer;
    const VkGeneratedCommandsInfoNV *pGeneratedCommandsInfo;
};

struct vkCmdPushConstants_params
{
    VkCommandBuffer commandBuffer;
    VkPipelineLayout layout;
    VkShaderStageFlags stageFlags;
    uint32_t offset;
    uint32_t size;
    const void *pValues;
};

struct vkCmdPushDescriptorSetKHR_params
{
    VkCommandBuffer commandBuffer;
    VkPipelineBindPoint pipelineBindPoint;
    VkPipelineLayout layout;
    uint32_t set;
    uint32_t descriptorWriteCount;
    const VkWriteDescriptorSet *pDescriptorWrites;
};

struct vkCmdPushDescriptorSetWithTemplateKHR_params
{
    VkCommandBuffer commandBuffer;
    VkDescriptorUpdateTemplate descriptorUpdateTemplate;
    VkPipelineLayout layout;
    uint32_t set;
    const void *pData;
};

struct vkCmdResetEvent_params
{
    VkCommandBuffer commandBuffer;
    VkEvent event;
    VkPipelineStageFlags stageMask;
};

struct vkCmdResetEvent2_params
{
    VkCommandBuffer commandBuffer;
    VkEvent event;
    VkPipelineStageFlags2 stageMask;
};

struct vkCmdResetEvent2KHR_params
{
    VkCommandBuffer commandBuffer;
    VkEvent event;
    VkPipelineStageFlags2 stageMask;
};

struct vkCmdResetQueryPool_params
{
    VkCommandBuffer commandBuffer;
    VkQueryPool queryPool;
    uint32_t firstQuery;
    uint32_t queryCount;
};

struct vkCmdResolveImage_params
{
    VkCommandBuffer commandBuffer;
    VkImage srcImage;
    VkImageLayout srcImageLayout;
    VkImage dstImage;
    VkImageLayout dstImageLayout;
    uint32_t regionCount;
    const VkImageResolve *pRegions;
};

struct vkCmdResolveImage2_params
{
    VkCommandBuffer commandBuffer;
    const VkResolveImageInfo2 *pResolveImageInfo;
};

struct vkCmdResolveImage2KHR_params
{
    VkCommandBuffer commandBuffer;
    const VkResolveImageInfo2 *pResolveImageInfo;
};

struct vkCmdSetBlendConstants_params
{
    VkCommandBuffer commandBuffer;
    const float *blendConstants;
};

struct vkCmdSetCheckpointNV_params
{
    VkCommandBuffer commandBuffer;
    const void *pCheckpointMarker;
};

struct vkCmdSetCoarseSampleOrderNV_params
{
    VkCommandBuffer commandBuffer;
    VkCoarseSampleOrderTypeNV sampleOrderType;
    uint32_t customSampleOrderCount;
    const VkCoarseSampleOrderCustomNV *pCustomSampleOrders;
};

struct vkCmdSetColorWriteEnableEXT_params
{
    VkCommandBuffer commandBuffer;
    uint32_t attachmentCount;
    const VkBool32 *pColorWriteEnables;
};

struct vkCmdSetCullMode_params
{
    VkCommandBuffer commandBuffer;
    VkCullModeFlags cullMode;
};

struct vkCmdSetCullModeEXT_params
{
    VkCommandBuffer commandBuffer;
    VkCullModeFlags cullMode;
};

struct vkCmdSetDepthBias_params
{
    VkCommandBuffer commandBuffer;
    float depthBiasConstantFactor;
    float depthBiasClamp;
    float depthBiasSlopeFactor;
};

struct vkCmdSetDepthBiasEnable_params
{
    VkCommandBuffer commandBuffer;
    VkBool32 depthBiasEnable;
};

struct vkCmdSetDepthBiasEnableEXT_params
{
    VkCommandBuffer commandBuffer;
    VkBool32 depthBiasEnable;
};

struct vkCmdSetDepthBounds_params
{
    VkCommandBuffer commandBuffer;
    float minDepthBounds;
    float maxDepthBounds;
};

struct vkCmdSetDepthBoundsTestEnable_params
{
    VkCommandBuffer commandBuffer;
    VkBool32 depthBoundsTestEnable;
};

struct vkCmdSetDepthBoundsTestEnableEXT_params
{
    VkCommandBuffer commandBuffer;
    VkBool32 depthBoundsTestEnable;
};

struct vkCmdSetDepthCompareOp_params
{
    VkCommandBuffer commandBuffer;
    VkCompareOp depthCompareOp;
};

struct vkCmdSetDepthCompareOpEXT_params
{
    VkCommandBuffer commandBuffer;
    VkCompareOp depthCompareOp;
};

struct vkCmdSetDepthTestEnable_params
{
    VkCommandBuffer commandBuffer;
    VkBool32 depthTestEnable;
};

struct vkCmdSetDepthTestEnableEXT_params
{
    VkCommandBuffer commandBuffer;
    VkBool32 depthTestEnable;
};

struct vkCmdSetDepthWriteEnable_params
{
    VkCommandBuffer commandBuffer;
    VkBool32 depthWriteEnable;
};

struct vkCmdSetDepthWriteEnableEXT_params
{
    VkCommandBuffer commandBuffer;
    VkBool32 depthWriteEnable;
};

struct vkCmdSetDeviceMask_params
{
    VkCommandBuffer commandBuffer;
    uint32_t deviceMask;
};

struct vkCmdSetDeviceMaskKHR_params
{
    VkCommandBuffer commandBuffer;
    uint32_t deviceMask;
};

struct vkCmdSetDiscardRectangleEXT_params
{
    VkCommandBuffer commandBuffer;
    uint32_t firstDiscardRectangle;
    uint32_t discardRectangleCount;
    const VkRect2D *pDiscardRectangles;
};

struct vkCmdSetEvent_params
{
    VkCommandBuffer commandBuffer;
    VkEvent event;
    VkPipelineStageFlags stageMask;
};

struct vkCmdSetEvent2_params
{
    VkCommandBuffer commandBuffer;
    VkEvent event;
    const VkDependencyInfo *pDependencyInfo;
};

struct vkCmdSetEvent2KHR_params
{
    VkCommandBuffer commandBuffer;
    VkEvent event;
    const VkDependencyInfo *pDependencyInfo;
};

struct vkCmdSetExclusiveScissorNV_params
{
    VkCommandBuffer commandBuffer;
    uint32_t firstExclusiveScissor;
    uint32_t exclusiveScissorCount;
    const VkRect2D *pExclusiveScissors;
};

struct vkCmdSetFragmentShadingRateEnumNV_params
{
    VkCommandBuffer commandBuffer;
    VkFragmentShadingRateNV shadingRate;
    const VkFragmentShadingRateCombinerOpKHR *combinerOps;
};

struct vkCmdSetFragmentShadingRateKHR_params
{
    VkCommandBuffer commandBuffer;
    const VkExtent2D *pFragmentSize;
    const VkFragmentShadingRateCombinerOpKHR *combinerOps;
};

struct vkCmdSetFrontFace_params
{
    VkCommandBuffer commandBuffer;
    VkFrontFace frontFace;
};

struct vkCmdSetFrontFaceEXT_params
{
    VkCommandBuffer commandBuffer;
    VkFrontFace frontFace;
};

struct vkCmdSetLineStippleEXT_params
{
    VkCommandBuffer commandBuffer;
    uint32_t lineStippleFactor;
    uint16_t lineStipplePattern;
};

struct vkCmdSetLineWidth_params
{
    VkCommandBuffer commandBuffer;
    float lineWidth;
};

struct vkCmdSetLogicOpEXT_params
{
    VkCommandBuffer commandBuffer;
    VkLogicOp logicOp;
};

struct vkCmdSetPatchControlPointsEXT_params
{
    VkCommandBuffer commandBuffer;
    uint32_t patchControlPoints;
};

struct vkCmdSetPerformanceMarkerINTEL_params
{
    VkCommandBuffer commandBuffer;
    const VkPerformanceMarkerInfoINTEL *pMarkerInfo;
};

struct vkCmdSetPerformanceOverrideINTEL_params
{
    VkCommandBuffer commandBuffer;
    const VkPerformanceOverrideInfoINTEL *pOverrideInfo;
};

struct vkCmdSetPerformanceStreamMarkerINTEL_params
{
    VkCommandBuffer commandBuffer;
    const VkPerformanceStreamMarkerInfoINTEL *pMarkerInfo;
};

struct vkCmdSetPrimitiveRestartEnable_params
{
    VkCommandBuffer commandBuffer;
    VkBool32 primitiveRestartEnable;
};

struct vkCmdSetPrimitiveRestartEnableEXT_params
{
    VkCommandBuffer commandBuffer;
    VkBool32 primitiveRestartEnable;
};

struct vkCmdSetPrimitiveTopology_params
{
    VkCommandBuffer commandBuffer;
    VkPrimitiveTopology primitiveTopology;
};

struct vkCmdSetPrimitiveTopologyEXT_params
{
    VkCommandBuffer commandBuffer;
    VkPrimitiveTopology primitiveTopology;
};

struct vkCmdSetRasterizerDiscardEnable_params
{
    VkCommandBuffer commandBuffer;
    VkBool32 rasterizerDiscardEnable;
};

struct vkCmdSetRasterizerDiscardEnableEXT_params
{
    VkCommandBuffer commandBuffer;
    VkBool32 rasterizerDiscardEnable;
};

struct vkCmdSetRayTracingPipelineStackSizeKHR_params
{
    VkCommandBuffer commandBuffer;
    uint32_t pipelineStackSize;
};

struct vkCmdSetSampleLocationsEXT_params
{
    VkCommandBuffer commandBuffer;
    const VkSampleLocationsInfoEXT *pSampleLocationsInfo;
};

struct vkCmdSetScissor_params
{
    VkCommandBuffer commandBuffer;
    uint32_t firstScissor;
    uint32_t scissorCount;
    const VkRect2D *pScissors;
};

struct vkCmdSetScissorWithCount_params
{
    VkCommandBuffer commandBuffer;
    uint32_t scissorCount;
    const VkRect2D *pScissors;
};

struct vkCmdSetScissorWithCountEXT_params
{
    VkCommandBuffer commandBuffer;
    uint32_t scissorCount;
    const VkRect2D *pScissors;
};

struct vkCmdSetStencilCompareMask_params
{
    VkCommandBuffer commandBuffer;
    VkStencilFaceFlags faceMask;
    uint32_t compareMask;
};

struct vkCmdSetStencilOp_params
{
    VkCommandBuffer commandBuffer;
    VkStencilFaceFlags faceMask;
    VkStencilOp failOp;
    VkStencilOp passOp;
    VkStencilOp depthFailOp;
    VkCompareOp compareOp;
};

struct vkCmdSetStencilOpEXT_params
{
    VkCommandBuffer commandBuffer;
    VkStencilFaceFlags faceMask;
    VkStencilOp failOp;
    VkStencilOp passOp;
    VkStencilOp depthFailOp;
    VkCompareOp compareOp;
};

struct vkCmdSetStencilReference_params
{
    VkCommandBuffer commandBuffer;
    VkStencilFaceFlags faceMask;
    uint32_t reference;
};

struct vkCmdSetStencilTestEnable_params
{
    VkCommandBuffer commandBuffer;
    VkBool32 stencilTestEnable;
};

struct vkCmdSetStencilTestEnableEXT_params
{
    VkCommandBuffer commandBuffer;
    VkBool32 stencilTestEnable;
};

struct vkCmdSetStencilWriteMask_params
{
    VkCommandBuffer commandBuffer;
    VkStencilFaceFlags faceMask;
    uint32_t writeMask;
};

struct vkCmdSetVertexInputEXT_params
{
    VkCommandBuffer commandBuffer;
    uint32_t vertexBindingDescriptionCount;
    const VkVertexInputBindingDescription2EXT *pVertexBindingDescriptions;
    uint32_t vertexAttributeDescriptionCount;
    const VkVertexInputAttributeDescription2EXT *pVertexAttributeDescriptions;
};

struct vkCmdSetViewport_params
{
    VkCommandBuffer commandBuffer;
    uint32_t firstViewport;
    uint32_t viewportCount;
    const VkViewport *pViewports;
};

struct vkCmdSetViewportShadingRatePaletteNV_params
{
    VkCommandBuffer commandBuffer;
    uint32_t firstViewport;
    uint32_t viewportCount;
    const VkShadingRatePaletteNV *pShadingRatePalettes;
};

struct vkCmdSetViewportWScalingNV_params
{
    VkCommandBuffer commandBuffer;
    uint32_t firstViewport;
    uint32_t viewportCount;
    const VkViewportWScalingNV *pViewportWScalings;
};

struct vkCmdSetViewportWithCount_params
{
    VkCommandBuffer commandBuffer;
    uint32_t viewportCount;
    const VkViewport *pViewports;
};

struct vkCmdSetViewportWithCountEXT_params
{
    VkCommandBuffer commandBuffer;
    uint32_t viewportCount;
    const VkViewport *pViewports;
};

struct vkCmdSubpassShadingHUAWEI_params
{
    VkCommandBuffer commandBuffer;
};

struct vkCmdTraceRaysIndirect2KHR_params
{
    VkCommandBuffer commandBuffer;
    VkDeviceAddress indirectDeviceAddress;
};

struct vkCmdTraceRaysIndirectKHR_params
{
    VkCommandBuffer commandBuffer;
    const VkStridedDeviceAddressRegionKHR *pRaygenShaderBindingTable;
    const VkStridedDeviceAddressRegionKHR *pMissShaderBindingTable;
    const VkStridedDeviceAddressRegionKHR *pHitShaderBindingTable;
    const VkStridedDeviceAddressRegionKHR *pCallableShaderBindingTable;
    VkDeviceAddress indirectDeviceAddress;
};

struct vkCmdTraceRaysKHR_params
{
    VkCommandBuffer commandBuffer;
    const VkStridedDeviceAddressRegionKHR *pRaygenShaderBindingTable;
    const VkStridedDeviceAddressRegionKHR *pMissShaderBindingTable;
    const VkStridedDeviceAddressRegionKHR *pHitShaderBindingTable;
    const VkStridedDeviceAddressRegionKHR *pCallableShaderBindingTable;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
};

struct vkCmdTraceRaysNV_params
{
    VkCommandBuffer commandBuffer;
    VkBuffer raygenShaderBindingTableBuffer;
    VkDeviceSize raygenShaderBindingOffset;
    VkBuffer missShaderBindingTableBuffer;
    VkDeviceSize missShaderBindingOffset;
    VkDeviceSize missShaderBindingStride;
    VkBuffer hitShaderBindingTableBuffer;
    VkDeviceSize hitShaderBindingOffset;
    VkDeviceSize hitShaderBindingStride;
    VkBuffer callableShaderBindingTableBuffer;
    VkDeviceSize callableShaderBindingOffset;
    VkDeviceSize callableShaderBindingStride;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
};

struct vkCmdUpdateBuffer_params
{
    VkCommandBuffer commandBuffer;
    VkBuffer dstBuffer;
    VkDeviceSize dstOffset;
    VkDeviceSize dataSize;
    const void *pData;
};

struct vkCmdWaitEvents_params
{
    VkCommandBuffer commandBuffer;
    uint32_t eventCount;
    const VkEvent *pEvents;
    VkPipelineStageFlags srcStageMask;
    VkPipelineStageFlags dstStageMask;
    uint32_t memoryBarrierCount;
    const VkMemoryBarrier *pMemoryBarriers;
    uint32_t bufferMemoryBarrierCount;
    const VkBufferMemoryBarrier *pBufferMemoryBarriers;
    uint32_t imageMemoryBarrierCount;
    const VkImageMemoryBarrier *pImageMemoryBarriers;
};

struct vkCmdWaitEvents2_params
{
    VkCommandBuffer commandBuffer;
    uint32_t eventCount;
    const VkEvent *pEvents;
    const VkDependencyInfo *pDependencyInfos;
};

struct vkCmdWaitEvents2KHR_params
{
    VkCommandBuffer commandBuffer;
    uint32_t eventCount;
    const VkEvent *pEvents;
    const VkDependencyInfo *pDependencyInfos;
};

struct vkCmdWriteAccelerationStructuresPropertiesKHR_params
{
    VkCommandBuffer commandBuffer;
    uint32_t accelerationStructureCount;
    const VkAccelerationStructureKHR *pAccelerationStructures;
    VkQueryType queryType;
    VkQueryPool queryPool;
    uint32_t firstQuery;
};

struct vkCmdWriteAccelerationStructuresPropertiesNV_params
{
    VkCommandBuffer commandBuffer;
    uint32_t accelerationStructureCount;
    const VkAccelerationStructureNV *pAccelerationStructures;
    VkQueryType queryType;
    VkQueryPool queryPool;
    uint32_t firstQuery;
};

struct vkCmdWriteBufferMarker2AMD_params
{
    VkCommandBuffer commandBuffer;
    VkPipelineStageFlags2 stage;
    VkBuffer dstBuffer;
    VkDeviceSize dstOffset;
    uint32_t marker;
};

struct vkCmdWriteBufferMarkerAMD_params
{
    VkCommandBuffer commandBuffer;
    VkPipelineStageFlagBits pipelineStage;
    VkBuffer dstBuffer;
    VkDeviceSize dstOffset;
    uint32_t marker;
};

struct vkCmdWriteTimestamp_params
{
    VkCommandBuffer commandBuffer;
    VkPipelineStageFlagBits pipelineStage;
    VkQueryPool queryPool;
    uint32_t query;
};

struct vkCmdWriteTimestamp2_params
{
    VkCommandBuffer commandBuffer;
    VkPipelineStageFlags2 stage;
    VkQueryPool queryPool;
    uint32_t query;
};

struct vkCmdWriteTimestamp2KHR_params
{
    VkCommandBuffer commandBuffer;
    VkPipelineStageFlags2 stage;
    VkQueryPool queryPool;
    uint32_t query;
};

struct vkCompileDeferredNV_params
{
    VkDevice device;
    VkPipeline pipeline;
    uint32_t shader;
};

struct vkCopyAccelerationStructureKHR_params
{
    VkDevice device;
    VkDeferredOperationKHR deferredOperation;
    const VkCopyAccelerationStructureInfoKHR *pInfo;
};

struct vkCopyAccelerationStructureToMemoryKHR_params
{
    VkDevice device;
    VkDeferredOperationKHR deferredOperation;
    const VkCopyAccelerationStructureToMemoryInfoKHR *pInfo;
};

struct vkCopyMemoryToAccelerationStructureKHR_params
{
    VkDevice device;
    VkDeferredOperationKHR deferredOperation;
    const VkCopyMemoryToAccelerationStructureInfoKHR *pInfo;
};

struct vkCreateAccelerationStructureKHR_params
{
    VkDevice device;
    const VkAccelerationStructureCreateInfoKHR *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkAccelerationStructureKHR *pAccelerationStructure;
};

struct vkCreateAccelerationStructureNV_params
{
    VkDevice device;
    const VkAccelerationStructureCreateInfoNV *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkAccelerationStructureNV *pAccelerationStructure;
};

struct vkCreateBuffer_params
{
    VkDevice device;
    const VkBufferCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkBuffer *pBuffer;
};

struct vkCreateBufferView_params
{
    VkDevice device;
    const VkBufferViewCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkBufferView *pView;
};

struct vkCreateCommandPool_params
{
    VkDevice device;
    const VkCommandPoolCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkCommandPool *pCommandPool;
};

struct vkCreateComputePipelines_params
{
    VkDevice device;
    VkPipelineCache pipelineCache;
    uint32_t createInfoCount;
    const VkComputePipelineCreateInfo *pCreateInfos;
    const VkAllocationCallbacks *pAllocator;
    VkPipeline *pPipelines;
};

struct vkCreateCuFunctionNVX_params
{
    VkDevice device;
    const VkCuFunctionCreateInfoNVX *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkCuFunctionNVX *pFunction;
};

struct vkCreateCuModuleNVX_params
{
    VkDevice device;
    const VkCuModuleCreateInfoNVX *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkCuModuleNVX *pModule;
};

struct vkCreateDebugReportCallbackEXT_params
{
    VkInstance instance;
    const VkDebugReportCallbackCreateInfoEXT *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkDebugReportCallbackEXT *pCallback;
};

struct vkCreateDebugUtilsMessengerEXT_params
{
    VkInstance instance;
    const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkDebugUtilsMessengerEXT *pMessenger;
};

struct vkCreateDeferredOperationKHR_params
{
    VkDevice device;
    const VkAllocationCallbacks *pAllocator;
    VkDeferredOperationKHR *pDeferredOperation;
};

struct vkCreateDescriptorPool_params
{
    VkDevice device;
    const VkDescriptorPoolCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkDescriptorPool *pDescriptorPool;
};

struct vkCreateDescriptorSetLayout_params
{
    VkDevice device;
    const VkDescriptorSetLayoutCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkDescriptorSetLayout *pSetLayout;
};

struct vkCreateDescriptorUpdateTemplate_params
{
    VkDevice device;
    const VkDescriptorUpdateTemplateCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkDescriptorUpdateTemplate *pDescriptorUpdateTemplate;
};

struct vkCreateDescriptorUpdateTemplateKHR_params
{
    VkDevice device;
    const VkDescriptorUpdateTemplateCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkDescriptorUpdateTemplate *pDescriptorUpdateTemplate;
};

struct vkCreateDevice_params
{
    VkPhysicalDevice physicalDevice;
    const VkDeviceCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkDevice *pDevice;
};

struct vkCreateEvent_params
{
    VkDevice device;
    const VkEventCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkEvent *pEvent;
};

struct vkCreateFence_params
{
    VkDevice device;
    const VkFenceCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkFence *pFence;
};

struct vkCreateFramebuffer_params
{
    VkDevice device;
    const VkFramebufferCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkFramebuffer *pFramebuffer;
};

struct vkCreateGraphicsPipelines_params
{
    VkDevice device;
    VkPipelineCache pipelineCache;
    uint32_t createInfoCount;
    const VkGraphicsPipelineCreateInfo *pCreateInfos;
    const VkAllocationCallbacks *pAllocator;
    VkPipeline *pPipelines;
};

struct vkCreateImage_params
{
    VkDevice device;
    const VkImageCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkImage *pImage;
};

struct vkCreateImageView_params
{
    VkDevice device;
    const VkImageViewCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkImageView *pView;
};

struct vkCreateIndirectCommandsLayoutNV_params
{
    VkDevice device;
    const VkIndirectCommandsLayoutCreateInfoNV *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkIndirectCommandsLayoutNV *pIndirectCommandsLayout;
};

struct vkCreateInstance_params
{
    const VkInstanceCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkInstance *pInstance;
};

struct vkCreatePipelineCache_params
{
    VkDevice device;
    const VkPipelineCacheCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkPipelineCache *pPipelineCache;
};

struct vkCreatePipelineLayout_params
{
    VkDevice device;
    const VkPipelineLayoutCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkPipelineLayout *pPipelineLayout;
};

struct vkCreatePrivateDataSlot_params
{
    VkDevice device;
    const VkPrivateDataSlotCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkPrivateDataSlot *pPrivateDataSlot;
};

struct vkCreatePrivateDataSlotEXT_params
{
    VkDevice device;
    const VkPrivateDataSlotCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkPrivateDataSlot *pPrivateDataSlot;
};

struct vkCreateQueryPool_params
{
    VkDevice device;
    const VkQueryPoolCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkQueryPool *pQueryPool;
};

struct vkCreateRayTracingPipelinesKHR_params
{
    VkDevice device;
    VkDeferredOperationKHR deferredOperation;
    VkPipelineCache pipelineCache;
    uint32_t createInfoCount;
    const VkRayTracingPipelineCreateInfoKHR *pCreateInfos;
    const VkAllocationCallbacks *pAllocator;
    VkPipeline *pPipelines;
};

struct vkCreateRayTracingPipelinesNV_params
{
    VkDevice device;
    VkPipelineCache pipelineCache;
    uint32_t createInfoCount;
    const VkRayTracingPipelineCreateInfoNV *pCreateInfos;
    const VkAllocationCallbacks *pAllocator;
    VkPipeline *pPipelines;
};

struct vkCreateRenderPass_params
{
    VkDevice device;
    const VkRenderPassCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkRenderPass *pRenderPass;
};

struct vkCreateRenderPass2_params
{
    VkDevice device;
    const VkRenderPassCreateInfo2 *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkRenderPass *pRenderPass;
};

struct vkCreateRenderPass2KHR_params
{
    VkDevice device;
    const VkRenderPassCreateInfo2 *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkRenderPass *pRenderPass;
};

struct vkCreateSampler_params
{
    VkDevice device;
    const VkSamplerCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkSampler *pSampler;
};

struct vkCreateSamplerYcbcrConversion_params
{
    VkDevice device;
    const VkSamplerYcbcrConversionCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkSamplerYcbcrConversion *pYcbcrConversion;
};

struct vkCreateSamplerYcbcrConversionKHR_params
{
    VkDevice device;
    const VkSamplerYcbcrConversionCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkSamplerYcbcrConversion *pYcbcrConversion;
};

struct vkCreateSemaphore_params
{
    VkDevice device;
    const VkSemaphoreCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkSemaphore *pSemaphore;
};

struct vkCreateShaderModule_params
{
    VkDevice device;
    const VkShaderModuleCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkShaderModule *pShaderModule;
};

struct vkCreateSwapchainKHR_params
{
    VkDevice device;
    const VkSwapchainCreateInfoKHR *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkSwapchainKHR *pSwapchain;
};

struct vkCreateValidationCacheEXT_params
{
    VkDevice device;
    const VkValidationCacheCreateInfoEXT *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkValidationCacheEXT *pValidationCache;
};

struct vkCreateWin32SurfaceKHR_params
{
    VkInstance instance;
    const VkWin32SurfaceCreateInfoKHR *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkSurfaceKHR *pSurface;
};

struct vkDebugMarkerSetObjectNameEXT_params
{
    VkDevice device;
    const VkDebugMarkerObjectNameInfoEXT *pNameInfo;
};

struct vkDebugMarkerSetObjectTagEXT_params
{
    VkDevice device;
    const VkDebugMarkerObjectTagInfoEXT *pTagInfo;
};

struct vkDebugReportMessageEXT_params
{
    VkInstance instance;
    VkDebugReportFlagsEXT flags;
    VkDebugReportObjectTypeEXT objectType;
    uint64_t object;
    size_t location;
    int32_t messageCode;
    const char *pLayerPrefix;
    const char *pMessage;
};

struct vkDeferredOperationJoinKHR_params
{
    VkDevice device;
    VkDeferredOperationKHR operation;
};

struct vkDestroyAccelerationStructureKHR_params
{
    VkDevice device;
    VkAccelerationStructureKHR accelerationStructure;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyAccelerationStructureNV_params
{
    VkDevice device;
    VkAccelerationStructureNV accelerationStructure;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyBuffer_params
{
    VkDevice device;
    VkBuffer buffer;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyBufferView_params
{
    VkDevice device;
    VkBufferView bufferView;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyCommandPool_params
{
    VkDevice device;
    VkCommandPool commandPool;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyCuFunctionNVX_params
{
    VkDevice device;
    VkCuFunctionNVX function;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyCuModuleNVX_params
{
    VkDevice device;
    VkCuModuleNVX module;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyDebugReportCallbackEXT_params
{
    VkInstance instance;
    VkDebugReportCallbackEXT callback;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyDebugUtilsMessengerEXT_params
{
    VkInstance instance;
    VkDebugUtilsMessengerEXT messenger;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyDeferredOperationKHR_params
{
    VkDevice device;
    VkDeferredOperationKHR operation;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyDescriptorPool_params
{
    VkDevice device;
    VkDescriptorPool descriptorPool;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyDescriptorSetLayout_params
{
    VkDevice device;
    VkDescriptorSetLayout descriptorSetLayout;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyDescriptorUpdateTemplate_params
{
    VkDevice device;
    VkDescriptorUpdateTemplate descriptorUpdateTemplate;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyDescriptorUpdateTemplateKHR_params
{
    VkDevice device;
    VkDescriptorUpdateTemplate descriptorUpdateTemplate;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyDevice_params
{
    VkDevice device;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyEvent_params
{
    VkDevice device;
    VkEvent event;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyFence_params
{
    VkDevice device;
    VkFence fence;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyFramebuffer_params
{
    VkDevice device;
    VkFramebuffer framebuffer;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyImage_params
{
    VkDevice device;
    VkImage image;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyImageView_params
{
    VkDevice device;
    VkImageView imageView;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyIndirectCommandsLayoutNV_params
{
    VkDevice device;
    VkIndirectCommandsLayoutNV indirectCommandsLayout;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyInstance_params
{
    VkInstance instance;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyPipeline_params
{
    VkDevice device;
    VkPipeline pipeline;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyPipelineCache_params
{
    VkDevice device;
    VkPipelineCache pipelineCache;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyPipelineLayout_params
{
    VkDevice device;
    VkPipelineLayout pipelineLayout;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyPrivateDataSlot_params
{
    VkDevice device;
    VkPrivateDataSlot privateDataSlot;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyPrivateDataSlotEXT_params
{
    VkDevice device;
    VkPrivateDataSlot privateDataSlot;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyQueryPool_params
{
    VkDevice device;
    VkQueryPool queryPool;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyRenderPass_params
{
    VkDevice device;
    VkRenderPass renderPass;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroySampler_params
{
    VkDevice device;
    VkSampler sampler;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroySamplerYcbcrConversion_params
{
    VkDevice device;
    VkSamplerYcbcrConversion ycbcrConversion;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroySamplerYcbcrConversionKHR_params
{
    VkDevice device;
    VkSamplerYcbcrConversion ycbcrConversion;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroySemaphore_params
{
    VkDevice device;
    VkSemaphore semaphore;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyShaderModule_params
{
    VkDevice device;
    VkShaderModule shaderModule;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroySurfaceKHR_params
{
    VkInstance instance;
    VkSurfaceKHR surface;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroySwapchainKHR_params
{
    VkDevice device;
    VkSwapchainKHR swapchain;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyValidationCacheEXT_params
{
    VkDevice device;
    VkValidationCacheEXT validationCache;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDeviceWaitIdle_params
{
    VkDevice device;
};

struct vkEndCommandBuffer_params
{
    VkCommandBuffer commandBuffer;
};

struct vkEnumerateDeviceExtensionProperties_params
{
    VkPhysicalDevice physicalDevice;
    const char *pLayerName;
    uint32_t *pPropertyCount;
    VkExtensionProperties *pProperties;
};

struct vkEnumerateDeviceLayerProperties_params
{
    VkPhysicalDevice physicalDevice;
    uint32_t *pPropertyCount;
    VkLayerProperties *pProperties;
};

struct vkEnumerateInstanceExtensionProperties_params
{
    const char *pLayerName;
    uint32_t *pPropertyCount;
    VkExtensionProperties *pProperties;
};

struct vkEnumerateInstanceVersion_params
{
    uint32_t *pApiVersion;
};

struct vkEnumeratePhysicalDeviceGroups_params
{
    VkInstance instance;
    uint32_t *pPhysicalDeviceGroupCount;
    VkPhysicalDeviceGroupProperties *pPhysicalDeviceGroupProperties;
};

struct vkEnumeratePhysicalDeviceGroupsKHR_params
{
    VkInstance instance;
    uint32_t *pPhysicalDeviceGroupCount;
    VkPhysicalDeviceGroupProperties *pPhysicalDeviceGroupProperties;
};

struct vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR_params
{
    VkPhysicalDevice physicalDevice;
    uint32_t queueFamilyIndex;
    uint32_t *pCounterCount;
    VkPerformanceCounterKHR *pCounters;
    VkPerformanceCounterDescriptionKHR *pCounterDescriptions;
};

struct vkEnumeratePhysicalDevices_params
{
    VkInstance instance;
    uint32_t *pPhysicalDeviceCount;
    VkPhysicalDevice *pPhysicalDevices;
};

struct vkFlushMappedMemoryRanges_params
{
    VkDevice device;
    uint32_t memoryRangeCount;
    const VkMappedMemoryRange *pMemoryRanges;
};

struct vkFreeCommandBuffers_params
{
    VkDevice device;
    VkCommandPool commandPool;
    uint32_t commandBufferCount;
    const VkCommandBuffer *pCommandBuffers;
};

struct vkFreeDescriptorSets_params
{
    VkDevice device;
    VkDescriptorPool descriptorPool;
    uint32_t descriptorSetCount;
    const VkDescriptorSet *pDescriptorSets;
};

struct vkFreeMemory_params
{
    VkDevice device;
    VkDeviceMemory memory;
    const VkAllocationCallbacks *pAllocator;
};

struct vkGetAccelerationStructureBuildSizesKHR_params
{
    VkDevice device;
    VkAccelerationStructureBuildTypeKHR buildType;
    const VkAccelerationStructureBuildGeometryInfoKHR *pBuildInfo;
    const uint32_t *pMaxPrimitiveCounts;
    VkAccelerationStructureBuildSizesInfoKHR *pSizeInfo;
};

struct vkGetAccelerationStructureDeviceAddressKHR_params
{
    VkDevice device;
    const VkAccelerationStructureDeviceAddressInfoKHR *pInfo;
    VkDeviceAddress result;
};

struct vkGetAccelerationStructureHandleNV_params
{
    VkDevice device;
    VkAccelerationStructureNV accelerationStructure;
    size_t dataSize;
    void *pData;
};

struct vkGetAccelerationStructureMemoryRequirementsNV_params
{
    VkDevice device;
    const VkAccelerationStructureMemoryRequirementsInfoNV *pInfo;
    VkMemoryRequirements2KHR *pMemoryRequirements;
};

struct vkGetBufferDeviceAddress_params
{
    VkDevice device;
    const VkBufferDeviceAddressInfo *pInfo;
    VkDeviceAddress result;
};

struct vkGetBufferDeviceAddressEXT_params
{
    VkDevice device;
    const VkBufferDeviceAddressInfo *pInfo;
    VkDeviceAddress result;
};

struct vkGetBufferDeviceAddressKHR_params
{
    VkDevice device;
    const VkBufferDeviceAddressInfo *pInfo;
    VkDeviceAddress result;
};

struct vkGetBufferMemoryRequirements_params
{
    VkDevice device;
    VkBuffer buffer;
    VkMemoryRequirements *pMemoryRequirements;
};

struct vkGetBufferMemoryRequirements2_params
{
    VkDevice device;
    const VkBufferMemoryRequirementsInfo2 *pInfo;
    VkMemoryRequirements2 *pMemoryRequirements;
};

struct vkGetBufferMemoryRequirements2KHR_params
{
    VkDevice device;
    const VkBufferMemoryRequirementsInfo2 *pInfo;
    VkMemoryRequirements2 *pMemoryRequirements;
};

struct vkGetBufferOpaqueCaptureAddress_params
{
    VkDevice device;
    const VkBufferDeviceAddressInfo *pInfo;
    uint64_t result;
};

struct vkGetBufferOpaqueCaptureAddressKHR_params
{
    VkDevice device;
    const VkBufferDeviceAddressInfo *pInfo;
    uint64_t result;
};

struct vkGetCalibratedTimestampsEXT_params
{
    VkDevice device;
    uint32_t timestampCount;
    const VkCalibratedTimestampInfoEXT *pTimestampInfos;
    uint64_t *pTimestamps;
    uint64_t *pMaxDeviation;
};

struct vkGetDeferredOperationMaxConcurrencyKHR_params
{
    VkDevice device;
    VkDeferredOperationKHR operation;
};

struct vkGetDeferredOperationResultKHR_params
{
    VkDevice device;
    VkDeferredOperationKHR operation;
};

struct vkGetDescriptorSetHostMappingVALVE_params
{
    VkDevice device;
    VkDescriptorSet descriptorSet;
    void **ppData;
};

struct vkGetDescriptorSetLayoutHostMappingInfoVALVE_params
{
    VkDevice device;
    const VkDescriptorSetBindingReferenceVALVE *pBindingReference;
    VkDescriptorSetLayoutHostMappingInfoVALVE *pHostMapping;
};

struct vkGetDescriptorSetLayoutSupport_params
{
    VkDevice device;
    const VkDescriptorSetLayoutCreateInfo *pCreateInfo;
    VkDescriptorSetLayoutSupport *pSupport;
};

struct vkGetDescriptorSetLayoutSupportKHR_params
{
    VkDevice device;
    const VkDescriptorSetLayoutCreateInfo *pCreateInfo;
    VkDescriptorSetLayoutSupport *pSupport;
};

struct vkGetDeviceAccelerationStructureCompatibilityKHR_params
{
    VkDevice device;
    const VkAccelerationStructureVersionInfoKHR *pVersionInfo;
    VkAccelerationStructureCompatibilityKHR *pCompatibility;
};

struct vkGetDeviceBufferMemoryRequirements_params
{
    VkDevice device;
    const VkDeviceBufferMemoryRequirements *pInfo;
    VkMemoryRequirements2 *pMemoryRequirements;
};

struct vkGetDeviceBufferMemoryRequirementsKHR_params
{
    VkDevice device;
    const VkDeviceBufferMemoryRequirements *pInfo;
    VkMemoryRequirements2 *pMemoryRequirements;
};

struct vkGetDeviceGroupPeerMemoryFeatures_params
{
    VkDevice device;
    uint32_t heapIndex;
    uint32_t localDeviceIndex;
    uint32_t remoteDeviceIndex;
    VkPeerMemoryFeatureFlags *pPeerMemoryFeatures;
};

struct vkGetDeviceGroupPeerMemoryFeaturesKHR_params
{
    VkDevice device;
    uint32_t heapIndex;
    uint32_t localDeviceIndex;
    uint32_t remoteDeviceIndex;
    VkPeerMemoryFeatureFlags *pPeerMemoryFeatures;
};

struct vkGetDeviceGroupPresentCapabilitiesKHR_params
{
    VkDevice device;
    VkDeviceGroupPresentCapabilitiesKHR *pDeviceGroupPresentCapabilities;
};

struct vkGetDeviceGroupSurfacePresentModesKHR_params
{
    VkDevice device;
    VkSurfaceKHR surface;
    VkDeviceGroupPresentModeFlagsKHR *pModes;
};

struct vkGetDeviceImageMemoryRequirements_params
{
    VkDevice device;
    const VkDeviceImageMemoryRequirements *pInfo;
    VkMemoryRequirements2 *pMemoryRequirements;
};

struct vkGetDeviceImageMemoryRequirementsKHR_params
{
    VkDevice device;
    const VkDeviceImageMemoryRequirements *pInfo;
    VkMemoryRequirements2 *pMemoryRequirements;
};

struct vkGetDeviceImageSparseMemoryRequirements_params
{
    VkDevice device;
    const VkDeviceImageMemoryRequirements *pInfo;
    uint32_t *pSparseMemoryRequirementCount;
    VkSparseImageMemoryRequirements2 *pSparseMemoryRequirements;
};

struct vkGetDeviceImageSparseMemoryRequirementsKHR_params
{
    VkDevice device;
    const VkDeviceImageMemoryRequirements *pInfo;
    uint32_t *pSparseMemoryRequirementCount;
    VkSparseImageMemoryRequirements2 *pSparseMemoryRequirements;
};

struct vkGetDeviceMemoryCommitment_params
{
    VkDevice device;
    VkDeviceMemory memory;
    VkDeviceSize *pCommittedMemoryInBytes;
};

struct vkGetDeviceMemoryOpaqueCaptureAddress_params
{
    VkDevice device;
    const VkDeviceMemoryOpaqueCaptureAddressInfo *pInfo;
    uint64_t result;
};

struct vkGetDeviceMemoryOpaqueCaptureAddressKHR_params
{
    VkDevice device;
    const VkDeviceMemoryOpaqueCaptureAddressInfo *pInfo;
    uint64_t result;
};

struct vkGetDeviceQueue_params
{
    VkDevice device;
    uint32_t queueFamilyIndex;
    uint32_t queueIndex;
    VkQueue *pQueue;
};

struct vkGetDeviceQueue2_params
{
    VkDevice device;
    const VkDeviceQueueInfo2 *pQueueInfo;
    VkQueue *pQueue;
};

struct vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI_params
{
    VkDevice device;
    VkRenderPass renderpass;
    VkExtent2D *pMaxWorkgroupSize;
};

struct vkGetDynamicRenderingTilePropertiesQCOM_params
{
    VkDevice device;
    const VkRenderingInfo *pRenderingInfo;
    VkTilePropertiesQCOM *pProperties;
};

struct vkGetEventStatus_params
{
    VkDevice device;
    VkEvent event;
};

struct vkGetFenceStatus_params
{
    VkDevice device;
    VkFence fence;
};

struct vkGetFramebufferTilePropertiesQCOM_params
{
    VkDevice device;
    VkFramebuffer framebuffer;
    uint32_t *pPropertiesCount;
    VkTilePropertiesQCOM *pProperties;
};

struct vkGetGeneratedCommandsMemoryRequirementsNV_params
{
    VkDevice device;
    const VkGeneratedCommandsMemoryRequirementsInfoNV *pInfo;
    VkMemoryRequirements2 *pMemoryRequirements;
};

struct vkGetImageMemoryRequirements_params
{
    VkDevice device;
    VkImage image;
    VkMemoryRequirements *pMemoryRequirements;
};

struct vkGetImageMemoryRequirements2_params
{
    VkDevice device;
    const VkImageMemoryRequirementsInfo2 *pInfo;
    VkMemoryRequirements2 *pMemoryRequirements;
};

struct vkGetImageMemoryRequirements2KHR_params
{
    VkDevice device;
    const VkImageMemoryRequirementsInfo2 *pInfo;
    VkMemoryRequirements2 *pMemoryRequirements;
};

struct vkGetImageSparseMemoryRequirements_params
{
    VkDevice device;
    VkImage image;
    uint32_t *pSparseMemoryRequirementCount;
    VkSparseImageMemoryRequirements *pSparseMemoryRequirements;
};

struct vkGetImageSparseMemoryRequirements2_params
{
    VkDevice device;
    const VkImageSparseMemoryRequirementsInfo2 *pInfo;
    uint32_t *pSparseMemoryRequirementCount;
    VkSparseImageMemoryRequirements2 *pSparseMemoryRequirements;
};

struct vkGetImageSparseMemoryRequirements2KHR_params
{
    VkDevice device;
    const VkImageSparseMemoryRequirementsInfo2 *pInfo;
    uint32_t *pSparseMemoryRequirementCount;
    VkSparseImageMemoryRequirements2 *pSparseMemoryRequirements;
};

struct vkGetImageSubresourceLayout_params
{
    VkDevice device;
    VkImage image;
    const VkImageSubresource *pSubresource;
    VkSubresourceLayout *pLayout;
};

struct vkGetImageSubresourceLayout2EXT_params
{
    VkDevice device;
    VkImage image;
    const VkImageSubresource2EXT *pSubresource;
    VkSubresourceLayout2EXT *pLayout;
};

struct vkGetImageViewAddressNVX_params
{
    VkDevice device;
    VkImageView imageView;
    VkImageViewAddressPropertiesNVX *pProperties;
};

struct vkGetImageViewHandleNVX_params
{
    VkDevice device;
    const VkImageViewHandleInfoNVX *pInfo;
};

struct vkGetMemoryHostPointerPropertiesEXT_params
{
    VkDevice device;
    VkExternalMemoryHandleTypeFlagBits handleType;
    const void *pHostPointer;
    VkMemoryHostPointerPropertiesEXT *pMemoryHostPointerProperties;
};

struct vkGetPerformanceParameterINTEL_params
{
    VkDevice device;
    VkPerformanceParameterTypeINTEL parameter;
    VkPerformanceValueINTEL *pValue;
};

struct vkGetPhysicalDeviceCalibrateableTimeDomainsEXT_params
{
    VkPhysicalDevice physicalDevice;
    uint32_t *pTimeDomainCount;
    VkTimeDomainEXT *pTimeDomains;
};

struct vkGetPhysicalDeviceCooperativeMatrixPropertiesNV_params
{
    VkPhysicalDevice physicalDevice;
    uint32_t *pPropertyCount;
    VkCooperativeMatrixPropertiesNV *pProperties;
};

struct vkGetPhysicalDeviceExternalBufferProperties_params
{
    VkPhysicalDevice physicalDevice;
    const VkPhysicalDeviceExternalBufferInfo *pExternalBufferInfo;
    VkExternalBufferProperties *pExternalBufferProperties;
};

struct vkGetPhysicalDeviceExternalBufferPropertiesKHR_params
{
    VkPhysicalDevice physicalDevice;
    const VkPhysicalDeviceExternalBufferInfo *pExternalBufferInfo;
    VkExternalBufferProperties *pExternalBufferProperties;
};

struct vkGetPhysicalDeviceExternalFenceProperties_params
{
    VkPhysicalDevice physicalDevice;
    const VkPhysicalDeviceExternalFenceInfo *pExternalFenceInfo;
    VkExternalFenceProperties *pExternalFenceProperties;
};

struct vkGetPhysicalDeviceExternalFencePropertiesKHR_params
{
    VkPhysicalDevice physicalDevice;
    const VkPhysicalDeviceExternalFenceInfo *pExternalFenceInfo;
    VkExternalFenceProperties *pExternalFenceProperties;
};

struct vkGetPhysicalDeviceExternalSemaphoreProperties_params
{
    VkPhysicalDevice physicalDevice;
    const VkPhysicalDeviceExternalSemaphoreInfo *pExternalSemaphoreInfo;
    VkExternalSemaphoreProperties *pExternalSemaphoreProperties;
};

struct vkGetPhysicalDeviceExternalSemaphorePropertiesKHR_params
{
    VkPhysicalDevice physicalDevice;
    const VkPhysicalDeviceExternalSemaphoreInfo *pExternalSemaphoreInfo;
    VkExternalSemaphoreProperties *pExternalSemaphoreProperties;
};

struct vkGetPhysicalDeviceFeatures_params
{
    VkPhysicalDevice physicalDevice;
    VkPhysicalDeviceFeatures *pFeatures;
};

struct vkGetPhysicalDeviceFeatures2_params
{
    VkPhysicalDevice physicalDevice;
    VkPhysicalDeviceFeatures2 *pFeatures;
};

struct vkGetPhysicalDeviceFeatures2KHR_params
{
    VkPhysicalDevice physicalDevice;
    VkPhysicalDeviceFeatures2 *pFeatures;
};

struct vkGetPhysicalDeviceFormatProperties_params
{
    VkPhysicalDevice physicalDevice;
    VkFormat format;
    VkFormatProperties *pFormatProperties;
};

struct vkGetPhysicalDeviceFormatProperties2_params
{
    VkPhysicalDevice physicalDevice;
    VkFormat format;
    VkFormatProperties2 *pFormatProperties;
};

struct vkGetPhysicalDeviceFormatProperties2KHR_params
{
    VkPhysicalDevice physicalDevice;
    VkFormat format;
    VkFormatProperties2 *pFormatProperties;
};

struct vkGetPhysicalDeviceFragmentShadingRatesKHR_params
{
    VkPhysicalDevice physicalDevice;
    uint32_t *pFragmentShadingRateCount;
    VkPhysicalDeviceFragmentShadingRateKHR *pFragmentShadingRates;
};

struct vkGetPhysicalDeviceImageFormatProperties_params
{
    VkPhysicalDevice physicalDevice;
    VkFormat format;
    VkImageType type;
    VkImageTiling tiling;
    VkImageUsageFlags usage;
    VkImageCreateFlags flags;
    VkImageFormatProperties *pImageFormatProperties;
};

struct vkGetPhysicalDeviceImageFormatProperties2_params
{
    VkPhysicalDevice physicalDevice;
    const VkPhysicalDeviceImageFormatInfo2 *pImageFormatInfo;
    VkImageFormatProperties2 *pImageFormatProperties;
};

struct vkGetPhysicalDeviceImageFormatProperties2KHR_params
{
    VkPhysicalDevice physicalDevice;
    const VkPhysicalDeviceImageFormatInfo2 *pImageFormatInfo;
    VkImageFormatProperties2 *pImageFormatProperties;
};

struct vkGetPhysicalDeviceMemoryProperties_params
{
    VkPhysicalDevice physicalDevice;
    VkPhysicalDeviceMemoryProperties *pMemoryProperties;
};

struct vkGetPhysicalDeviceMemoryProperties2_params
{
    VkPhysicalDevice physicalDevice;
    VkPhysicalDeviceMemoryProperties2 *pMemoryProperties;
};

struct vkGetPhysicalDeviceMemoryProperties2KHR_params
{
    VkPhysicalDevice physicalDevice;
    VkPhysicalDeviceMemoryProperties2 *pMemoryProperties;
};

struct vkGetPhysicalDeviceMultisamplePropertiesEXT_params
{
    VkPhysicalDevice physicalDevice;
    VkSampleCountFlagBits samples;
    VkMultisamplePropertiesEXT *pMultisampleProperties;
};

struct vkGetPhysicalDevicePresentRectanglesKHR_params
{
    VkPhysicalDevice physicalDevice;
    VkSurfaceKHR surface;
    uint32_t *pRectCount;
    VkRect2D *pRects;
};

struct vkGetPhysicalDeviceProperties_params
{
    VkPhysicalDevice physicalDevice;
    VkPhysicalDeviceProperties *pProperties;
};

struct vkGetPhysicalDeviceProperties2_params
{
    VkPhysicalDevice physicalDevice;
    VkPhysicalDeviceProperties2 *pProperties;
};

struct vkGetPhysicalDeviceProperties2KHR_params
{
    VkPhysicalDevice physicalDevice;
    VkPhysicalDeviceProperties2 *pProperties;
};

struct vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR_params
{
    VkPhysicalDevice physicalDevice;
    const VkQueryPoolPerformanceCreateInfoKHR *pPerformanceQueryCreateInfo;
    uint32_t *pNumPasses;
};

struct vkGetPhysicalDeviceQueueFamilyProperties_params
{
    VkPhysicalDevice physicalDevice;
    uint32_t *pQueueFamilyPropertyCount;
    VkQueueFamilyProperties *pQueueFamilyProperties;
};

struct vkGetPhysicalDeviceQueueFamilyProperties2_params
{
    VkPhysicalDevice physicalDevice;
    uint32_t *pQueueFamilyPropertyCount;
    VkQueueFamilyProperties2 *pQueueFamilyProperties;
};

struct vkGetPhysicalDeviceQueueFamilyProperties2KHR_params
{
    VkPhysicalDevice physicalDevice;
    uint32_t *pQueueFamilyPropertyCount;
    VkQueueFamilyProperties2 *pQueueFamilyProperties;
};

struct vkGetPhysicalDeviceSparseImageFormatProperties_params
{
    VkPhysicalDevice physicalDevice;
    VkFormat format;
    VkImageType type;
    VkSampleCountFlagBits samples;
    VkImageUsageFlags usage;
    VkImageTiling tiling;
    uint32_t *pPropertyCount;
    VkSparseImageFormatProperties *pProperties;
};

struct vkGetPhysicalDeviceSparseImageFormatProperties2_params
{
    VkPhysicalDevice physicalDevice;
    const VkPhysicalDeviceSparseImageFormatInfo2 *pFormatInfo;
    uint32_t *pPropertyCount;
    VkSparseImageFormatProperties2 *pProperties;
};

struct vkGetPhysicalDeviceSparseImageFormatProperties2KHR_params
{
    VkPhysicalDevice physicalDevice;
    const VkPhysicalDeviceSparseImageFormatInfo2 *pFormatInfo;
    uint32_t *pPropertyCount;
    VkSparseImageFormatProperties2 *pProperties;
};

struct vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV_params
{
    VkPhysicalDevice physicalDevice;
    uint32_t *pCombinationCount;
    VkFramebufferMixedSamplesCombinationNV *pCombinations;
};

struct vkGetPhysicalDeviceSurfaceCapabilities2KHR_params
{
    VkPhysicalDevice physicalDevice;
    const VkPhysicalDeviceSurfaceInfo2KHR *pSurfaceInfo;
    VkSurfaceCapabilities2KHR *pSurfaceCapabilities;
};

struct vkGetPhysicalDeviceSurfaceCapabilitiesKHR_params
{
    VkPhysicalDevice physicalDevice;
    VkSurfaceKHR surface;
    VkSurfaceCapabilitiesKHR *pSurfaceCapabilities;
};

struct vkGetPhysicalDeviceSurfaceFormats2KHR_params
{
    VkPhysicalDevice physicalDevice;
    const VkPhysicalDeviceSurfaceInfo2KHR *pSurfaceInfo;
    uint32_t *pSurfaceFormatCount;
    VkSurfaceFormat2KHR *pSurfaceFormats;
};

struct vkGetPhysicalDeviceSurfaceFormatsKHR_params
{
    VkPhysicalDevice physicalDevice;
    VkSurfaceKHR surface;
    uint32_t *pSurfaceFormatCount;
    VkSurfaceFormatKHR *pSurfaceFormats;
};

struct vkGetPhysicalDeviceSurfacePresentModesKHR_params
{
    VkPhysicalDevice physicalDevice;
    VkSurfaceKHR surface;
    uint32_t *pPresentModeCount;
    VkPresentModeKHR *pPresentModes;
};

struct vkGetPhysicalDeviceSurfaceSupportKHR_params
{
    VkPhysicalDevice physicalDevice;
    uint32_t queueFamilyIndex;
    VkSurfaceKHR surface;
    VkBool32 *pSupported;
};

struct vkGetPhysicalDeviceToolProperties_params
{
    VkPhysicalDevice physicalDevice;
    uint32_t *pToolCount;
    VkPhysicalDeviceToolProperties *pToolProperties;
};

struct vkGetPhysicalDeviceToolPropertiesEXT_params
{
    VkPhysicalDevice physicalDevice;
    uint32_t *pToolCount;
    VkPhysicalDeviceToolProperties *pToolProperties;
};

struct vkGetPhysicalDeviceWin32PresentationSupportKHR_params
{
    VkPhysicalDevice physicalDevice;
    uint32_t queueFamilyIndex;
};

struct vkGetPipelineCacheData_params
{
    VkDevice device;
    VkPipelineCache pipelineCache;
    size_t *pDataSize;
    void *pData;
};

struct vkGetPipelineExecutableInternalRepresentationsKHR_params
{
    VkDevice device;
    const VkPipelineExecutableInfoKHR *pExecutableInfo;
    uint32_t *pInternalRepresentationCount;
    VkPipelineExecutableInternalRepresentationKHR *pInternalRepresentations;
};

struct vkGetPipelineExecutablePropertiesKHR_params
{
    VkDevice device;
    const VkPipelineInfoKHR *pPipelineInfo;
    uint32_t *pExecutableCount;
    VkPipelineExecutablePropertiesKHR *pProperties;
};

struct vkGetPipelineExecutableStatisticsKHR_params
{
    VkDevice device;
    const VkPipelineExecutableInfoKHR *pExecutableInfo;
    uint32_t *pStatisticCount;
    VkPipelineExecutableStatisticKHR *pStatistics;
};

struct vkGetPipelinePropertiesEXT_params
{
    VkDevice device;
    const VkPipelineInfoEXT *pPipelineInfo;
    VkBaseOutStructure *pPipelineProperties;
};

struct vkGetPrivateData_params
{
    VkDevice device;
    VkObjectType objectType;
    uint64_t objectHandle;
    VkPrivateDataSlot privateDataSlot;
    uint64_t *pData;
};

struct vkGetPrivateDataEXT_params
{
    VkDevice device;
    VkObjectType objectType;
    uint64_t objectHandle;
    VkPrivateDataSlot privateDataSlot;
    uint64_t *pData;
};

struct vkGetQueryPoolResults_params
{
    VkDevice device;
    VkQueryPool queryPool;
    uint32_t firstQuery;
    uint32_t queryCount;
    size_t dataSize;
    void *pData;
    VkDeviceSize stride;
    VkQueryResultFlags flags;
};

struct vkGetQueueCheckpointData2NV_params
{
    VkQueue queue;
    uint32_t *pCheckpointDataCount;
    VkCheckpointData2NV *pCheckpointData;
};

struct vkGetQueueCheckpointDataNV_params
{
    VkQueue queue;
    uint32_t *pCheckpointDataCount;
    VkCheckpointDataNV *pCheckpointData;
};

struct vkGetRayTracingCaptureReplayShaderGroupHandlesKHR_params
{
    VkDevice device;
    VkPipeline pipeline;
    uint32_t firstGroup;
    uint32_t groupCount;
    size_t dataSize;
    void *pData;
};

struct vkGetRayTracingShaderGroupHandlesKHR_params
{
    VkDevice device;
    VkPipeline pipeline;
    uint32_t firstGroup;
    uint32_t groupCount;
    size_t dataSize;
    void *pData;
};

struct vkGetRayTracingShaderGroupHandlesNV_params
{
    VkDevice device;
    VkPipeline pipeline;
    uint32_t firstGroup;
    uint32_t groupCount;
    size_t dataSize;
    void *pData;
};

struct vkGetRayTracingShaderGroupStackSizeKHR_params
{
    VkDevice device;
    VkPipeline pipeline;
    uint32_t group;
    VkShaderGroupShaderKHR groupShader;
};

struct vkGetRenderAreaGranularity_params
{
    VkDevice device;
    VkRenderPass renderPass;
    VkExtent2D *pGranularity;
};

struct vkGetSemaphoreCounterValue_params
{
    VkDevice device;
    VkSemaphore semaphore;
    uint64_t *pValue;
};

struct vkGetSemaphoreCounterValueKHR_params
{
    VkDevice device;
    VkSemaphore semaphore;
    uint64_t *pValue;
};

struct vkGetShaderInfoAMD_params
{
    VkDevice device;
    VkPipeline pipeline;
    VkShaderStageFlagBits shaderStage;
    VkShaderInfoTypeAMD infoType;
    size_t *pInfoSize;
    void *pInfo;
};

struct vkGetShaderModuleCreateInfoIdentifierEXT_params
{
    VkDevice device;
    const VkShaderModuleCreateInfo *pCreateInfo;
    VkShaderModuleIdentifierEXT *pIdentifier;
};

struct vkGetShaderModuleIdentifierEXT_params
{
    VkDevice device;
    VkShaderModule shaderModule;
    VkShaderModuleIdentifierEXT *pIdentifier;
};

struct vkGetSwapchainImagesKHR_params
{
    VkDevice device;
    VkSwapchainKHR swapchain;
    uint32_t *pSwapchainImageCount;
    VkImage *pSwapchainImages;
};

struct vkGetValidationCacheDataEXT_params
{
    VkDevice device;
    VkValidationCacheEXT validationCache;
    size_t *pDataSize;
    void *pData;
};

struct vkInitializePerformanceApiINTEL_params
{
    VkDevice device;
    const VkInitializePerformanceApiInfoINTEL *pInitializeInfo;
};

struct vkInvalidateMappedMemoryRanges_params
{
    VkDevice device;
    uint32_t memoryRangeCount;
    const VkMappedMemoryRange *pMemoryRanges;
};

struct vkMapMemory_params
{
    VkDevice device;
    VkDeviceMemory memory;
    VkDeviceSize offset;
    VkDeviceSize size;
    VkMemoryMapFlags flags;
    void **ppData;
};

struct vkMergePipelineCaches_params
{
    VkDevice device;
    VkPipelineCache dstCache;
    uint32_t srcCacheCount;
    const VkPipelineCache *pSrcCaches;
};

struct vkMergeValidationCachesEXT_params
{
    VkDevice device;
    VkValidationCacheEXT dstCache;
    uint32_t srcCacheCount;
    const VkValidationCacheEXT *pSrcCaches;
};

struct vkQueueBeginDebugUtilsLabelEXT_params
{
    VkQueue queue;
    const VkDebugUtilsLabelEXT *pLabelInfo;
};

struct vkQueueBindSparse_params
{
    VkQueue queue;
    uint32_t bindInfoCount;
    const VkBindSparseInfo *pBindInfo;
    VkFence fence;
};

struct vkQueueEndDebugUtilsLabelEXT_params
{
    VkQueue queue;
};

struct vkQueueInsertDebugUtilsLabelEXT_params
{
    VkQueue queue;
    const VkDebugUtilsLabelEXT *pLabelInfo;
};

struct vkQueuePresentKHR_params
{
    VkQueue queue;
    const VkPresentInfoKHR *pPresentInfo;
};

struct vkQueueSetPerformanceConfigurationINTEL_params
{
    VkQueue queue;
    VkPerformanceConfigurationINTEL configuration;
};

struct vkQueueSubmit_params
{
    VkQueue queue;
    uint32_t submitCount;
    const VkSubmitInfo *pSubmits;
    VkFence fence;
};

struct vkQueueSubmit2_params
{
    VkQueue queue;
    uint32_t submitCount;
    const VkSubmitInfo2 *pSubmits;
    VkFence fence;
};

struct vkQueueSubmit2KHR_params
{
    VkQueue queue;
    uint32_t submitCount;
    const VkSubmitInfo2 *pSubmits;
    VkFence fence;
};

struct vkQueueWaitIdle_params
{
    VkQueue queue;
};

struct vkReleasePerformanceConfigurationINTEL_params
{
    VkDevice device;
    VkPerformanceConfigurationINTEL configuration;
};

struct vkReleaseProfilingLockKHR_params
{
    VkDevice device;
};

struct vkResetCommandBuffer_params
{
    VkCommandBuffer commandBuffer;
    VkCommandBufferResetFlags flags;
};

struct vkResetCommandPool_params
{
    VkDevice device;
    VkCommandPool commandPool;
    VkCommandPoolResetFlags flags;
};

struct vkResetDescriptorPool_params
{
    VkDevice device;
    VkDescriptorPool descriptorPool;
    VkDescriptorPoolResetFlags flags;
};

struct vkResetEvent_params
{
    VkDevice device;
    VkEvent event;
};

struct vkResetFences_params
{
    VkDevice device;
    uint32_t fenceCount;
    const VkFence *pFences;
};

struct vkResetQueryPool_params
{
    VkDevice device;
    VkQueryPool queryPool;
    uint32_t firstQuery;
    uint32_t queryCount;
};

struct vkResetQueryPoolEXT_params
{
    VkDevice device;
    VkQueryPool queryPool;
    uint32_t firstQuery;
    uint32_t queryCount;
};

struct vkSetDebugUtilsObjectNameEXT_params
{
    VkDevice device;
    const VkDebugUtilsObjectNameInfoEXT *pNameInfo;
};

struct vkSetDebugUtilsObjectTagEXT_params
{
    VkDevice device;
    const VkDebugUtilsObjectTagInfoEXT *pTagInfo;
};

struct vkSetDeviceMemoryPriorityEXT_params
{
    VkDevice device;
    VkDeviceMemory memory;
    float priority;
};

struct vkSetEvent_params
{
    VkDevice device;
    VkEvent event;
};

struct vkSetPrivateData_params
{
    VkDevice device;
    VkObjectType objectType;
    uint64_t objectHandle;
    VkPrivateDataSlot privateDataSlot;
    uint64_t data;
};

struct vkSetPrivateDataEXT_params
{
    VkDevice device;
    VkObjectType objectType;
    uint64_t objectHandle;
    VkPrivateDataSlot privateDataSlot;
    uint64_t data;
};

struct vkSignalSemaphore_params
{
    VkDevice device;
    const VkSemaphoreSignalInfo *pSignalInfo;
};

struct vkSignalSemaphoreKHR_params
{
    VkDevice device;
    const VkSemaphoreSignalInfo *pSignalInfo;
};

struct vkSubmitDebugUtilsMessageEXT_params
{
    VkInstance instance;
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity;
    VkDebugUtilsMessageTypeFlagsEXT messageTypes;
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData;
};

struct vkTrimCommandPool_params
{
    VkDevice device;
    VkCommandPool commandPool;
    VkCommandPoolTrimFlags flags;
};

struct vkTrimCommandPoolKHR_params
{
    VkDevice device;
    VkCommandPool commandPool;
    VkCommandPoolTrimFlags flags;
};

struct vkUninitializePerformanceApiINTEL_params
{
    VkDevice device;
};

struct vkUnmapMemory_params
{
    VkDevice device;
    VkDeviceMemory memory;
};

struct vkUpdateDescriptorSetWithTemplate_params
{
    VkDevice device;
    VkDescriptorSet descriptorSet;
    VkDescriptorUpdateTemplate descriptorUpdateTemplate;
    const void *pData;
};

struct vkUpdateDescriptorSetWithTemplateKHR_params
{
    VkDevice device;
    VkDescriptorSet descriptorSet;
    VkDescriptorUpdateTemplate descriptorUpdateTemplate;
    const void *pData;
};

struct vkUpdateDescriptorSets_params
{
    VkDevice device;
    uint32_t descriptorWriteCount;
    const VkWriteDescriptorSet *pDescriptorWrites;
    uint32_t descriptorCopyCount;
    const VkCopyDescriptorSet *pDescriptorCopies;
};

struct vkWaitForFences_params
{
    VkDevice device;
    uint32_t fenceCount;
    const VkFence *pFences;
    VkBool32 waitAll;
    uint64_t timeout;
};

struct vkWaitForPresentKHR_params
{
    VkDevice device;
    VkSwapchainKHR swapchain;
    uint64_t presentId;
    uint64_t timeout;
};

struct vkWaitSemaphores_params
{
    VkDevice device;
    const VkSemaphoreWaitInfo *pWaitInfo;
    uint64_t timeout;
};

struct vkWaitSemaphoresKHR_params
{
    VkDevice device;
    const VkSemaphoreWaitInfo *pWaitInfo;
    uint64_t timeout;
};

struct vkWriteAccelerationStructuresPropertiesKHR_params
{
    VkDevice device;
    uint32_t accelerationStructureCount;
    const VkAccelerationStructureKHR *pAccelerationStructures;
    VkQueryType queryType;
    size_t dataSize;
    void *pData;
    size_t stride;
};

#include "poppack.h"

#endif /* __WINE_VULKAN_LOADER_THUNKS_H */
