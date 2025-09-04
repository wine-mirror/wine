/* Automatically generated from Vulkan vk.xml and video.xml; DO NOT EDIT!
 *
 * This file is generated from Vulkan vk.xml file covered
 * by the following copyright and permission notice:
 *
 * Copyright 2015-2025 The Khronos Group Inc.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * and from Vulkan video.xml file covered
 * by the following copyright and permission notice:
 *
 * Copyright 2021-2025 The Khronos Group Inc.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __WINE_VULKAN_LOADER_THUNKS_H
#define __WINE_VULKAN_LOADER_THUNKS_H

enum unix_call
{
    unix_init,
    unix_is_available_instance_function,
    unix_is_available_device_function,
    unix_vkAcquireNextImage2KHR,
    unix_vkAcquireNextImageKHR,
    unix_vkAcquirePerformanceConfigurationINTEL,
    unix_vkAcquireProfilingLockKHR,
    unix_vkAllocateCommandBuffers,
    unix_vkAllocateDescriptorSets,
    unix_vkAllocateMemory,
    unix_vkAntiLagUpdateAMD,
    unix_vkBeginCommandBuffer,
    unix_vkBindAccelerationStructureMemoryNV,
    unix_vkBindBufferMemory,
    unix_vkBindBufferMemory2,
    unix_vkBindBufferMemory2KHR,
    unix_vkBindDataGraphPipelineSessionMemoryARM,
    unix_vkBindImageMemory,
    unix_vkBindImageMemory2,
    unix_vkBindImageMemory2KHR,
    unix_vkBindOpticalFlowSessionImageNV,
    unix_vkBindTensorMemoryARM,
    unix_vkBindVideoSessionMemoryKHR,
    unix_vkBuildAccelerationStructuresKHR,
    unix_vkBuildMicromapsEXT,
    unix_vkCmdBeginConditionalRenderingEXT,
    unix_vkCmdBeginDebugUtilsLabelEXT,
    unix_vkCmdBeginPerTileExecutionQCOM,
    unix_vkCmdBeginQuery,
    unix_vkCmdBeginQueryIndexedEXT,
    unix_vkCmdBeginRenderPass,
    unix_vkCmdBeginRenderPass2,
    unix_vkCmdBeginRenderPass2KHR,
    unix_vkCmdBeginRendering,
    unix_vkCmdBeginRenderingKHR,
    unix_vkCmdBeginTransformFeedbackEXT,
    unix_vkCmdBeginVideoCodingKHR,
    unix_vkCmdBindDescriptorBufferEmbeddedSamplers2EXT,
    unix_vkCmdBindDescriptorBufferEmbeddedSamplersEXT,
    unix_vkCmdBindDescriptorBuffersEXT,
    unix_vkCmdBindDescriptorSets,
    unix_vkCmdBindDescriptorSets2,
    unix_vkCmdBindDescriptorSets2KHR,
    unix_vkCmdBindIndexBuffer,
    unix_vkCmdBindIndexBuffer2,
    unix_vkCmdBindIndexBuffer2KHR,
    unix_vkCmdBindInvocationMaskHUAWEI,
    unix_vkCmdBindPipeline,
    unix_vkCmdBindPipelineShaderGroupNV,
    unix_vkCmdBindShadersEXT,
    unix_vkCmdBindShadingRateImageNV,
    unix_vkCmdBindTileMemoryQCOM,
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
    unix_vkCmdBuildClusterAccelerationStructureIndirectNV,
    unix_vkCmdBuildMicromapsEXT,
    unix_vkCmdBuildPartitionedAccelerationStructuresNV,
    unix_vkCmdClearAttachments,
    unix_vkCmdClearColorImage,
    unix_vkCmdClearDepthStencilImage,
    unix_vkCmdControlVideoCodingKHR,
    unix_vkCmdConvertCooperativeVectorMatrixNV,
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
    unix_vkCmdCopyMemoryIndirectNV,
    unix_vkCmdCopyMemoryToAccelerationStructureKHR,
    unix_vkCmdCopyMemoryToImageIndirectNV,
    unix_vkCmdCopyMemoryToMicromapEXT,
    unix_vkCmdCopyMicromapEXT,
    unix_vkCmdCopyMicromapToMemoryEXT,
    unix_vkCmdCopyQueryPoolResults,
    unix_vkCmdCopyTensorARM,
    unix_vkCmdCuLaunchKernelNVX,
    unix_vkCmdDebugMarkerBeginEXT,
    unix_vkCmdDebugMarkerEndEXT,
    unix_vkCmdDebugMarkerInsertEXT,
    unix_vkCmdDecodeVideoKHR,
    unix_vkCmdDecompressMemoryIndirectCountNV,
    unix_vkCmdDecompressMemoryNV,
    unix_vkCmdDispatch,
    unix_vkCmdDispatchBase,
    unix_vkCmdDispatchBaseKHR,
    unix_vkCmdDispatchDataGraphARM,
    unix_vkCmdDispatchIndirect,
    unix_vkCmdDispatchTileQCOM,
    unix_vkCmdDraw,
    unix_vkCmdDrawClusterHUAWEI,
    unix_vkCmdDrawClusterIndirectHUAWEI,
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
    unix_vkCmdDrawMeshTasksEXT,
    unix_vkCmdDrawMeshTasksIndirectCountEXT,
    unix_vkCmdDrawMeshTasksIndirectCountNV,
    unix_vkCmdDrawMeshTasksIndirectEXT,
    unix_vkCmdDrawMeshTasksIndirectNV,
    unix_vkCmdDrawMeshTasksNV,
    unix_vkCmdDrawMultiEXT,
    unix_vkCmdDrawMultiIndexedEXT,
    unix_vkCmdEncodeVideoKHR,
    unix_vkCmdEndConditionalRenderingEXT,
    unix_vkCmdEndDebugUtilsLabelEXT,
    unix_vkCmdEndPerTileExecutionQCOM,
    unix_vkCmdEndQuery,
    unix_vkCmdEndQueryIndexedEXT,
    unix_vkCmdEndRenderPass,
    unix_vkCmdEndRenderPass2,
    unix_vkCmdEndRenderPass2KHR,
    unix_vkCmdEndRendering,
    unix_vkCmdEndRendering2EXT,
    unix_vkCmdEndRenderingKHR,
    unix_vkCmdEndTransformFeedbackEXT,
    unix_vkCmdEndVideoCodingKHR,
    unix_vkCmdExecuteCommands,
    unix_vkCmdExecuteGeneratedCommandsEXT,
    unix_vkCmdExecuteGeneratedCommandsNV,
    unix_vkCmdFillBuffer,
    unix_vkCmdInsertDebugUtilsLabelEXT,
    unix_vkCmdNextSubpass,
    unix_vkCmdNextSubpass2,
    unix_vkCmdNextSubpass2KHR,
    unix_vkCmdOpticalFlowExecuteNV,
    unix_vkCmdPipelineBarrier,
    unix_vkCmdPipelineBarrier2,
    unix_vkCmdPipelineBarrier2KHR,
    unix_vkCmdPreprocessGeneratedCommandsEXT,
    unix_vkCmdPreprocessGeneratedCommandsNV,
    unix_vkCmdPushConstants,
    unix_vkCmdPushConstants2,
    unix_vkCmdPushConstants2KHR,
    unix_vkCmdPushDescriptorSet,
    unix_vkCmdPushDescriptorSet2,
    unix_vkCmdPushDescriptorSet2KHR,
    unix_vkCmdPushDescriptorSetKHR,
    unix_vkCmdPushDescriptorSetWithTemplate,
    unix_vkCmdPushDescriptorSetWithTemplate2,
    unix_vkCmdPushDescriptorSetWithTemplate2KHR,
    unix_vkCmdPushDescriptorSetWithTemplateKHR,
    unix_vkCmdResetEvent,
    unix_vkCmdResetEvent2,
    unix_vkCmdResetEvent2KHR,
    unix_vkCmdResetQueryPool,
    unix_vkCmdResolveImage,
    unix_vkCmdResolveImage2,
    unix_vkCmdResolveImage2KHR,
    unix_vkCmdSetAlphaToCoverageEnableEXT,
    unix_vkCmdSetAlphaToOneEnableEXT,
    unix_vkCmdSetAttachmentFeedbackLoopEnableEXT,
    unix_vkCmdSetBlendConstants,
    unix_vkCmdSetCheckpointNV,
    unix_vkCmdSetCoarseSampleOrderNV,
    unix_vkCmdSetColorBlendAdvancedEXT,
    unix_vkCmdSetColorBlendEnableEXT,
    unix_vkCmdSetColorBlendEquationEXT,
    unix_vkCmdSetColorWriteEnableEXT,
    unix_vkCmdSetColorWriteMaskEXT,
    unix_vkCmdSetConservativeRasterizationModeEXT,
    unix_vkCmdSetCoverageModulationModeNV,
    unix_vkCmdSetCoverageModulationTableEnableNV,
    unix_vkCmdSetCoverageModulationTableNV,
    unix_vkCmdSetCoverageReductionModeNV,
    unix_vkCmdSetCoverageToColorEnableNV,
    unix_vkCmdSetCoverageToColorLocationNV,
    unix_vkCmdSetCullMode,
    unix_vkCmdSetCullModeEXT,
    unix_vkCmdSetDepthBias,
    unix_vkCmdSetDepthBias2EXT,
    unix_vkCmdSetDepthBiasEnable,
    unix_vkCmdSetDepthBiasEnableEXT,
    unix_vkCmdSetDepthBounds,
    unix_vkCmdSetDepthBoundsTestEnable,
    unix_vkCmdSetDepthBoundsTestEnableEXT,
    unix_vkCmdSetDepthClampEnableEXT,
    unix_vkCmdSetDepthClampRangeEXT,
    unix_vkCmdSetDepthClipEnableEXT,
    unix_vkCmdSetDepthClipNegativeOneToOneEXT,
    unix_vkCmdSetDepthCompareOp,
    unix_vkCmdSetDepthCompareOpEXT,
    unix_vkCmdSetDepthTestEnable,
    unix_vkCmdSetDepthTestEnableEXT,
    unix_vkCmdSetDepthWriteEnable,
    unix_vkCmdSetDepthWriteEnableEXT,
    unix_vkCmdSetDescriptorBufferOffsets2EXT,
    unix_vkCmdSetDescriptorBufferOffsetsEXT,
    unix_vkCmdSetDeviceMask,
    unix_vkCmdSetDeviceMaskKHR,
    unix_vkCmdSetDiscardRectangleEXT,
    unix_vkCmdSetDiscardRectangleEnableEXT,
    unix_vkCmdSetDiscardRectangleModeEXT,
    unix_vkCmdSetEvent,
    unix_vkCmdSetEvent2,
    unix_vkCmdSetEvent2KHR,
    unix_vkCmdSetExclusiveScissorEnableNV,
    unix_vkCmdSetExclusiveScissorNV,
    unix_vkCmdSetExtraPrimitiveOverestimationSizeEXT,
    unix_vkCmdSetFragmentShadingRateEnumNV,
    unix_vkCmdSetFragmentShadingRateKHR,
    unix_vkCmdSetFrontFace,
    unix_vkCmdSetFrontFaceEXT,
    unix_vkCmdSetLineRasterizationModeEXT,
    unix_vkCmdSetLineStipple,
    unix_vkCmdSetLineStippleEXT,
    unix_vkCmdSetLineStippleEnableEXT,
    unix_vkCmdSetLineStippleKHR,
    unix_vkCmdSetLineWidth,
    unix_vkCmdSetLogicOpEXT,
    unix_vkCmdSetLogicOpEnableEXT,
    unix_vkCmdSetPatchControlPointsEXT,
    unix_vkCmdSetPerformanceMarkerINTEL,
    unix_vkCmdSetPerformanceOverrideINTEL,
    unix_vkCmdSetPerformanceStreamMarkerINTEL,
    unix_vkCmdSetPolygonModeEXT,
    unix_vkCmdSetPrimitiveRestartEnable,
    unix_vkCmdSetPrimitiveRestartEnableEXT,
    unix_vkCmdSetPrimitiveTopology,
    unix_vkCmdSetPrimitiveTopologyEXT,
    unix_vkCmdSetProvokingVertexModeEXT,
    unix_vkCmdSetRasterizationSamplesEXT,
    unix_vkCmdSetRasterizationStreamEXT,
    unix_vkCmdSetRasterizerDiscardEnable,
    unix_vkCmdSetRasterizerDiscardEnableEXT,
    unix_vkCmdSetRayTracingPipelineStackSizeKHR,
    unix_vkCmdSetRenderingAttachmentLocations,
    unix_vkCmdSetRenderingAttachmentLocationsKHR,
    unix_vkCmdSetRenderingInputAttachmentIndices,
    unix_vkCmdSetRenderingInputAttachmentIndicesKHR,
    unix_vkCmdSetRepresentativeFragmentTestEnableNV,
    unix_vkCmdSetSampleLocationsEXT,
    unix_vkCmdSetSampleLocationsEnableEXT,
    unix_vkCmdSetSampleMaskEXT,
    unix_vkCmdSetScissor,
    unix_vkCmdSetScissorWithCount,
    unix_vkCmdSetScissorWithCountEXT,
    unix_vkCmdSetShadingRateImageEnableNV,
    unix_vkCmdSetStencilCompareMask,
    unix_vkCmdSetStencilOp,
    unix_vkCmdSetStencilOpEXT,
    unix_vkCmdSetStencilReference,
    unix_vkCmdSetStencilTestEnable,
    unix_vkCmdSetStencilTestEnableEXT,
    unix_vkCmdSetStencilWriteMask,
    unix_vkCmdSetTessellationDomainOriginEXT,
    unix_vkCmdSetVertexInputEXT,
    unix_vkCmdSetViewport,
    unix_vkCmdSetViewportShadingRatePaletteNV,
    unix_vkCmdSetViewportSwizzleNV,
    unix_vkCmdSetViewportWScalingEnableNV,
    unix_vkCmdSetViewportWScalingNV,
    unix_vkCmdSetViewportWithCount,
    unix_vkCmdSetViewportWithCountEXT,
    unix_vkCmdSubpassShadingHUAWEI,
    unix_vkCmdTraceRaysIndirect2KHR,
    unix_vkCmdTraceRaysIndirectKHR,
    unix_vkCmdTraceRaysKHR,
    unix_vkCmdTraceRaysNV,
    unix_vkCmdUpdateBuffer,
    unix_vkCmdUpdatePipelineIndirectBufferNV,
    unix_vkCmdWaitEvents,
    unix_vkCmdWaitEvents2,
    unix_vkCmdWaitEvents2KHR,
    unix_vkCmdWriteAccelerationStructuresPropertiesKHR,
    unix_vkCmdWriteAccelerationStructuresPropertiesNV,
    unix_vkCmdWriteBufferMarker2AMD,
    unix_vkCmdWriteBufferMarkerAMD,
    unix_vkCmdWriteMicromapsPropertiesEXT,
    unix_vkCmdWriteTimestamp,
    unix_vkCmdWriteTimestamp2,
    unix_vkCmdWriteTimestamp2KHR,
    unix_vkCompileDeferredNV,
    unix_vkConvertCooperativeVectorMatrixNV,
    unix_vkCopyAccelerationStructureKHR,
    unix_vkCopyAccelerationStructureToMemoryKHR,
    unix_vkCopyImageToImage,
    unix_vkCopyImageToImageEXT,
    unix_vkCopyImageToMemory,
    unix_vkCopyImageToMemoryEXT,
    unix_vkCopyMemoryToAccelerationStructureKHR,
    unix_vkCopyMemoryToImage,
    unix_vkCopyMemoryToImageEXT,
    unix_vkCopyMemoryToMicromapEXT,
    unix_vkCopyMicromapEXT,
    unix_vkCopyMicromapToMemoryEXT,
    unix_vkCreateAccelerationStructureKHR,
    unix_vkCreateAccelerationStructureNV,
    unix_vkCreateBuffer,
    unix_vkCreateBufferView,
    unix_vkCreateCommandPool,
    unix_vkCreateComputePipelines,
    unix_vkCreateCuFunctionNVX,
    unix_vkCreateCuModuleNVX,
    unix_vkCreateDataGraphPipelineSessionARM,
    unix_vkCreateDataGraphPipelinesARM,
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
    unix_vkCreateIndirectCommandsLayoutEXT,
    unix_vkCreateIndirectCommandsLayoutNV,
    unix_vkCreateIndirectExecutionSetEXT,
    unix_vkCreateInstance,
    unix_vkCreateMicromapEXT,
    unix_vkCreateOpticalFlowSessionNV,
    unix_vkCreatePipelineBinariesKHR,
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
    unix_vkCreateShadersEXT,
    unix_vkCreateSwapchainKHR,
    unix_vkCreateTensorARM,
    unix_vkCreateTensorViewARM,
    unix_vkCreateValidationCacheEXT,
    unix_vkCreateVideoSessionKHR,
    unix_vkCreateVideoSessionParametersKHR,
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
    unix_vkDestroyDataGraphPipelineSessionARM,
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
    unix_vkDestroyIndirectCommandsLayoutEXT,
    unix_vkDestroyIndirectCommandsLayoutNV,
    unix_vkDestroyIndirectExecutionSetEXT,
    unix_vkDestroyInstance,
    unix_vkDestroyMicromapEXT,
    unix_vkDestroyOpticalFlowSessionNV,
    unix_vkDestroyPipeline,
    unix_vkDestroyPipelineBinaryKHR,
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
    unix_vkDestroyShaderEXT,
    unix_vkDestroyShaderModule,
    unix_vkDestroySurfaceKHR,
    unix_vkDestroySwapchainKHR,
    unix_vkDestroyTensorARM,
    unix_vkDestroyTensorViewARM,
    unix_vkDestroyValidationCacheEXT,
    unix_vkDestroyVideoSessionKHR,
    unix_vkDestroyVideoSessionParametersKHR,
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
    unix_vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT,
    unix_vkGetBufferDeviceAddress,
    unix_vkGetBufferDeviceAddressEXT,
    unix_vkGetBufferDeviceAddressKHR,
    unix_vkGetBufferMemoryRequirements,
    unix_vkGetBufferMemoryRequirements2,
    unix_vkGetBufferMemoryRequirements2KHR,
    unix_vkGetBufferOpaqueCaptureAddress,
    unix_vkGetBufferOpaqueCaptureAddressKHR,
    unix_vkGetBufferOpaqueCaptureDescriptorDataEXT,
    unix_vkGetCalibratedTimestampsEXT,
    unix_vkGetCalibratedTimestampsKHR,
    unix_vkGetClusterAccelerationStructureBuildSizesNV,
    unix_vkGetDataGraphPipelineAvailablePropertiesARM,
    unix_vkGetDataGraphPipelinePropertiesARM,
    unix_vkGetDataGraphPipelineSessionBindPointRequirementsARM,
    unix_vkGetDataGraphPipelineSessionMemoryRequirementsARM,
    unix_vkGetDeferredOperationMaxConcurrencyKHR,
    unix_vkGetDeferredOperationResultKHR,
    unix_vkGetDescriptorEXT,
    unix_vkGetDescriptorSetHostMappingVALVE,
    unix_vkGetDescriptorSetLayoutBindingOffsetEXT,
    unix_vkGetDescriptorSetLayoutHostMappingInfoVALVE,
    unix_vkGetDescriptorSetLayoutSizeEXT,
    unix_vkGetDescriptorSetLayoutSupport,
    unix_vkGetDescriptorSetLayoutSupportKHR,
    unix_vkGetDeviceAccelerationStructureCompatibilityKHR,
    unix_vkGetDeviceBufferMemoryRequirements,
    unix_vkGetDeviceBufferMemoryRequirementsKHR,
    unix_vkGetDeviceFaultInfoEXT,
    unix_vkGetDeviceGroupPeerMemoryFeatures,
    unix_vkGetDeviceGroupPeerMemoryFeaturesKHR,
    unix_vkGetDeviceGroupPresentCapabilitiesKHR,
    unix_vkGetDeviceGroupSurfacePresentModesKHR,
    unix_vkGetDeviceImageMemoryRequirements,
    unix_vkGetDeviceImageMemoryRequirementsKHR,
    unix_vkGetDeviceImageSparseMemoryRequirements,
    unix_vkGetDeviceImageSparseMemoryRequirementsKHR,
    unix_vkGetDeviceImageSubresourceLayout,
    unix_vkGetDeviceImageSubresourceLayoutKHR,
    unix_vkGetDeviceMemoryCommitment,
    unix_vkGetDeviceMemoryOpaqueCaptureAddress,
    unix_vkGetDeviceMemoryOpaqueCaptureAddressKHR,
    unix_vkGetDeviceMicromapCompatibilityEXT,
    unix_vkGetDeviceQueue,
    unix_vkGetDeviceQueue2,
    unix_vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI,
    unix_vkGetDeviceTensorMemoryRequirementsARM,
    unix_vkGetDynamicRenderingTilePropertiesQCOM,
    unix_vkGetEncodedVideoSessionParametersKHR,
    unix_vkGetEventStatus,
    unix_vkGetFenceStatus,
    unix_vkGetFramebufferTilePropertiesQCOM,
    unix_vkGetGeneratedCommandsMemoryRequirementsEXT,
    unix_vkGetGeneratedCommandsMemoryRequirementsNV,
    unix_vkGetImageMemoryRequirements,
    unix_vkGetImageMemoryRequirements2,
    unix_vkGetImageMemoryRequirements2KHR,
    unix_vkGetImageOpaqueCaptureDescriptorDataEXT,
    unix_vkGetImageSparseMemoryRequirements,
    unix_vkGetImageSparseMemoryRequirements2,
    unix_vkGetImageSparseMemoryRequirements2KHR,
    unix_vkGetImageSubresourceLayout,
    unix_vkGetImageSubresourceLayout2,
    unix_vkGetImageSubresourceLayout2EXT,
    unix_vkGetImageSubresourceLayout2KHR,
    unix_vkGetImageViewAddressNVX,
    unix_vkGetImageViewHandle64NVX,
    unix_vkGetImageViewHandleNVX,
    unix_vkGetImageViewOpaqueCaptureDescriptorDataEXT,
    unix_vkGetLatencyTimingsNV,
    unix_vkGetMemoryHostPointerPropertiesEXT,
    unix_vkGetMemoryWin32HandleKHR,
    unix_vkGetMemoryWin32HandlePropertiesKHR,
    unix_vkGetMicromapBuildSizesEXT,
    unix_vkGetPartitionedAccelerationStructuresBuildSizesNV,
    unix_vkGetPerformanceParameterINTEL,
    unix_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT,
    unix_vkGetPhysicalDeviceCalibrateableTimeDomainsKHR,
    unix_vkGetPhysicalDeviceCooperativeMatrixFlexibleDimensionsPropertiesNV,
    unix_vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR,
    unix_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV,
    unix_vkGetPhysicalDeviceCooperativeVectorPropertiesNV,
    unix_vkGetPhysicalDeviceExternalBufferProperties,
    unix_vkGetPhysicalDeviceExternalBufferPropertiesKHR,
    unix_vkGetPhysicalDeviceExternalFenceProperties,
    unix_vkGetPhysicalDeviceExternalFencePropertiesKHR,
    unix_vkGetPhysicalDeviceExternalSemaphoreProperties,
    unix_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR,
    unix_vkGetPhysicalDeviceExternalTensorPropertiesARM,
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
    unix_vkGetPhysicalDeviceOpticalFlowImageFormatsNV,
    unix_vkGetPhysicalDevicePresentRectanglesKHR,
    unix_vkGetPhysicalDeviceProperties,
    unix_vkGetPhysicalDeviceProperties2,
    unix_vkGetPhysicalDeviceProperties2KHR,
    unix_vkGetPhysicalDeviceQueueFamilyDataGraphProcessingEnginePropertiesARM,
    unix_vkGetPhysicalDeviceQueueFamilyDataGraphPropertiesARM,
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
    unix_vkGetPhysicalDeviceVideoCapabilitiesKHR,
    unix_vkGetPhysicalDeviceVideoEncodeQualityLevelPropertiesKHR,
    unix_vkGetPhysicalDeviceVideoFormatPropertiesKHR,
    unix_vkGetPhysicalDeviceWin32PresentationSupportKHR,
    unix_vkGetPipelineBinaryDataKHR,
    unix_vkGetPipelineCacheData,
    unix_vkGetPipelineExecutableInternalRepresentationsKHR,
    unix_vkGetPipelineExecutablePropertiesKHR,
    unix_vkGetPipelineExecutableStatisticsKHR,
    unix_vkGetPipelineIndirectDeviceAddressNV,
    unix_vkGetPipelineIndirectMemoryRequirementsNV,
    unix_vkGetPipelineKeyKHR,
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
    unix_vkGetRenderingAreaGranularity,
    unix_vkGetRenderingAreaGranularityKHR,
    unix_vkGetSamplerOpaqueCaptureDescriptorDataEXT,
    unix_vkGetSemaphoreCounterValue,
    unix_vkGetSemaphoreCounterValueKHR,
    unix_vkGetSemaphoreWin32HandleKHR,
    unix_vkGetShaderBinaryDataEXT,
    unix_vkGetShaderInfoAMD,
    unix_vkGetShaderModuleCreateInfoIdentifierEXT,
    unix_vkGetShaderModuleIdentifierEXT,
    unix_vkGetSwapchainImagesKHR,
    unix_vkGetTensorMemoryRequirementsARM,
    unix_vkGetTensorOpaqueCaptureDescriptorDataARM,
    unix_vkGetTensorViewOpaqueCaptureDescriptorDataARM,
    unix_vkGetValidationCacheDataEXT,
    unix_vkGetVideoSessionMemoryRequirementsKHR,
    unix_vkImportSemaphoreWin32HandleKHR,
    unix_vkInitializePerformanceApiINTEL,
    unix_vkInvalidateMappedMemoryRanges,
    unix_vkLatencySleepNV,
    unix_vkMapMemory,
    unix_vkMapMemory2,
    unix_vkMapMemory2KHR,
    unix_vkMergePipelineCaches,
    unix_vkMergeValidationCachesEXT,
    unix_vkQueueBeginDebugUtilsLabelEXT,
    unix_vkQueueBindSparse,
    unix_vkQueueEndDebugUtilsLabelEXT,
    unix_vkQueueInsertDebugUtilsLabelEXT,
    unix_vkQueueNotifyOutOfBandNV,
    unix_vkQueuePresentKHR,
    unix_vkQueueSetPerformanceConfigurationINTEL,
    unix_vkQueueSubmit,
    unix_vkQueueSubmit2,
    unix_vkQueueSubmit2KHR,
    unix_vkQueueWaitIdle,
    unix_vkReleaseCapturedPipelineDataKHR,
    unix_vkReleasePerformanceConfigurationINTEL,
    unix_vkReleaseProfilingLockKHR,
    unix_vkReleaseSwapchainImagesEXT,
    unix_vkReleaseSwapchainImagesKHR,
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
    unix_vkSetHdrMetadataEXT,
    unix_vkSetLatencyMarkerNV,
    unix_vkSetLatencySleepModeNV,
    unix_vkSetPrivateData,
    unix_vkSetPrivateDataEXT,
    unix_vkSignalSemaphore,
    unix_vkSignalSemaphoreKHR,
    unix_vkSubmitDebugUtilsMessageEXT,
    unix_vkTransitionImageLayout,
    unix_vkTransitionImageLayoutEXT,
    unix_vkTrimCommandPool,
    unix_vkTrimCommandPoolKHR,
    unix_vkUninitializePerformanceApiINTEL,
    unix_vkUnmapMemory,
    unix_vkUnmapMemory2,
    unix_vkUnmapMemory2KHR,
    unix_vkUpdateDescriptorSetWithTemplate,
    unix_vkUpdateDescriptorSetWithTemplateKHR,
    unix_vkUpdateDescriptorSets,
    unix_vkUpdateIndirectExecutionSetPipelineEXT,
    unix_vkUpdateIndirectExecutionSetShaderEXT,
    unix_vkUpdateVideoSessionParametersKHR,
    unix_vkWaitForFences,
    unix_vkWaitForPresent2KHR,
    unix_vkWaitForPresentKHR,
    unix_vkWaitSemaphores,
    unix_vkWaitSemaphoresKHR,
    unix_vkWriteAccelerationStructuresPropertiesKHR,
    unix_vkWriteMicromapsPropertiesEXT,
    unix_count,
};

struct vkAcquireNextImage2KHR_params
{
    VkDevice device;
    const VkAcquireNextImageInfoKHR *pAcquireInfo;
    uint32_t *pImageIndex;
    VkResult result;
};

struct vkAcquireNextImageKHR_params
{
    VkDevice device;
    VkSwapchainKHR DECLSPEC_ALIGN(8) swapchain;
    uint64_t DECLSPEC_ALIGN(8) timeout;
    VkSemaphore DECLSPEC_ALIGN(8) semaphore;
    VkFence DECLSPEC_ALIGN(8) fence;
    uint32_t *pImageIndex;
    VkResult result;
};

struct vkAcquirePerformanceConfigurationINTEL_params
{
    VkDevice device;
    const VkPerformanceConfigurationAcquireInfoINTEL *pAcquireInfo;
    VkPerformanceConfigurationINTEL *pConfiguration;
    VkResult result;
};

struct vkAcquireProfilingLockKHR_params
{
    VkDevice device;
    const VkAcquireProfilingLockInfoKHR *pInfo;
    VkResult result;
};

struct vkAllocateCommandBuffers_params
{
    VkDevice device;
    const VkCommandBufferAllocateInfo *pAllocateInfo;
    VkCommandBuffer *pCommandBuffers;
    VkResult result;
};

struct vkAllocateDescriptorSets_params
{
    VkDevice device;
    const VkDescriptorSetAllocateInfo *pAllocateInfo;
    VkDescriptorSet *pDescriptorSets;
    VkResult result;
};

struct vkAllocateMemory_params
{
    VkDevice device;
    const VkMemoryAllocateInfo *pAllocateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkDeviceMemory *pMemory;
    VkResult result;
};

struct vkAntiLagUpdateAMD_params
{
    VkDevice device;
    const VkAntiLagDataAMD *pData;
};

struct vkBeginCommandBuffer_params
{
    VkCommandBuffer commandBuffer;
    const VkCommandBufferBeginInfo *pBeginInfo;
    VkResult result;
};

struct vkBindAccelerationStructureMemoryNV_params
{
    VkDevice device;
    uint32_t bindInfoCount;
    const VkBindAccelerationStructureMemoryInfoNV *pBindInfos;
    VkResult result;
};

struct vkBindBufferMemory_params
{
    VkDevice device;
    VkBuffer DECLSPEC_ALIGN(8) buffer;
    VkDeviceMemory DECLSPEC_ALIGN(8) memory;
    VkDeviceSize DECLSPEC_ALIGN(8) memoryOffset;
    VkResult result;
};

struct vkBindBufferMemory2_params
{
    VkDevice device;
    uint32_t bindInfoCount;
    const VkBindBufferMemoryInfo *pBindInfos;
    VkResult result;
};

struct vkBindBufferMemory2KHR_params
{
    VkDevice device;
    uint32_t bindInfoCount;
    const VkBindBufferMemoryInfo *pBindInfos;
    VkResult result;
};

struct vkBindDataGraphPipelineSessionMemoryARM_params
{
    VkDevice device;
    uint32_t bindInfoCount;
    const VkBindDataGraphPipelineSessionMemoryInfoARM *pBindInfos;
    VkResult result;
};

struct vkBindImageMemory_params
{
    VkDevice device;
    VkImage DECLSPEC_ALIGN(8) image;
    VkDeviceMemory DECLSPEC_ALIGN(8) memory;
    VkDeviceSize DECLSPEC_ALIGN(8) memoryOffset;
    VkResult result;
};

struct vkBindImageMemory2_params
{
    VkDevice device;
    uint32_t bindInfoCount;
    const VkBindImageMemoryInfo *pBindInfos;
    VkResult result;
};

struct vkBindImageMemory2KHR_params
{
    VkDevice device;
    uint32_t bindInfoCount;
    const VkBindImageMemoryInfo *pBindInfos;
    VkResult result;
};

struct vkBindOpticalFlowSessionImageNV_params
{
    VkDevice device;
    VkOpticalFlowSessionNV DECLSPEC_ALIGN(8) session;
    VkOpticalFlowSessionBindingPointNV bindingPoint;
    VkImageView DECLSPEC_ALIGN(8) view;
    VkImageLayout layout;
    VkResult result;
};

struct vkBindTensorMemoryARM_params
{
    VkDevice device;
    uint32_t bindInfoCount;
    const VkBindTensorMemoryInfoARM *pBindInfos;
    VkResult result;
};

struct vkBindVideoSessionMemoryKHR_params
{
    VkDevice device;
    VkVideoSessionKHR DECLSPEC_ALIGN(8) videoSession;
    uint32_t bindSessionMemoryInfoCount;
    const VkBindVideoSessionMemoryInfoKHR *pBindSessionMemoryInfos;
    VkResult result;
};

struct vkBuildAccelerationStructuresKHR_params
{
    VkDevice device;
    VkDeferredOperationKHR DECLSPEC_ALIGN(8) deferredOperation;
    uint32_t infoCount;
    const VkAccelerationStructureBuildGeometryInfoKHR *pInfos;
    const VkAccelerationStructureBuildRangeInfoKHR * const*ppBuildRangeInfos;
    VkResult result;
};

struct vkBuildMicromapsEXT_params
{
    VkDevice device;
    VkDeferredOperationKHR DECLSPEC_ALIGN(8) deferredOperation;
    uint32_t infoCount;
    const VkMicromapBuildInfoEXT *pInfos;
    VkResult result;
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

struct vkCmdBeginPerTileExecutionQCOM_params
{
    VkCommandBuffer commandBuffer;
    const VkPerTileBeginInfoQCOM *pPerTileBeginInfo;
};

struct vkCmdBeginQuery_params
{
    VkCommandBuffer commandBuffer;
    VkQueryPool DECLSPEC_ALIGN(8) queryPool;
    uint32_t query;
    VkQueryControlFlags flags;
};

struct vkCmdBeginQueryIndexedEXT_params
{
    VkCommandBuffer commandBuffer;
    VkQueryPool DECLSPEC_ALIGN(8) queryPool;
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

struct vkCmdBeginVideoCodingKHR_params
{
    VkCommandBuffer commandBuffer;
    const VkVideoBeginCodingInfoKHR *pBeginInfo;
};

struct vkCmdBindDescriptorBufferEmbeddedSamplers2EXT_params
{
    VkCommandBuffer commandBuffer;
    const VkBindDescriptorBufferEmbeddedSamplersInfoEXT *pBindDescriptorBufferEmbeddedSamplersInfo;
};

struct vkCmdBindDescriptorBufferEmbeddedSamplersEXT_params
{
    VkCommandBuffer commandBuffer;
    VkPipelineBindPoint pipelineBindPoint;
    VkPipelineLayout DECLSPEC_ALIGN(8) layout;
    uint32_t set;
};

struct vkCmdBindDescriptorBuffersEXT_params
{
    VkCommandBuffer commandBuffer;
    uint32_t bufferCount;
    const VkDescriptorBufferBindingInfoEXT *pBindingInfos;
};

struct vkCmdBindDescriptorSets_params
{
    VkCommandBuffer commandBuffer;
    VkPipelineBindPoint pipelineBindPoint;
    VkPipelineLayout DECLSPEC_ALIGN(8) layout;
    uint32_t firstSet;
    uint32_t descriptorSetCount;
    const VkDescriptorSet *pDescriptorSets;
    uint32_t dynamicOffsetCount;
    const uint32_t *pDynamicOffsets;
};

struct vkCmdBindDescriptorSets2_params
{
    VkCommandBuffer commandBuffer;
    const VkBindDescriptorSetsInfo *pBindDescriptorSetsInfo;
};

struct vkCmdBindDescriptorSets2KHR_params
{
    VkCommandBuffer commandBuffer;
    const VkBindDescriptorSetsInfo *pBindDescriptorSetsInfo;
};

struct vkCmdBindIndexBuffer_params
{
    VkCommandBuffer commandBuffer;
    VkBuffer DECLSPEC_ALIGN(8) buffer;
    VkDeviceSize DECLSPEC_ALIGN(8) offset;
    VkIndexType indexType;
};

struct vkCmdBindIndexBuffer2_params
{
    VkCommandBuffer commandBuffer;
    VkBuffer DECLSPEC_ALIGN(8) buffer;
    VkDeviceSize DECLSPEC_ALIGN(8) offset;
    VkDeviceSize DECLSPEC_ALIGN(8) size;
    VkIndexType indexType;
};

struct vkCmdBindIndexBuffer2KHR_params
{
    VkCommandBuffer commandBuffer;
    VkBuffer DECLSPEC_ALIGN(8) buffer;
    VkDeviceSize DECLSPEC_ALIGN(8) offset;
    VkDeviceSize DECLSPEC_ALIGN(8) size;
    VkIndexType indexType;
};

struct vkCmdBindInvocationMaskHUAWEI_params
{
    VkCommandBuffer commandBuffer;
    VkImageView DECLSPEC_ALIGN(8) imageView;
    VkImageLayout imageLayout;
};

struct vkCmdBindPipeline_params
{
    VkCommandBuffer commandBuffer;
    VkPipelineBindPoint pipelineBindPoint;
    VkPipeline DECLSPEC_ALIGN(8) pipeline;
};

struct vkCmdBindPipelineShaderGroupNV_params
{
    VkCommandBuffer commandBuffer;
    VkPipelineBindPoint pipelineBindPoint;
    VkPipeline DECLSPEC_ALIGN(8) pipeline;
    uint32_t groupIndex;
};

struct vkCmdBindShadersEXT_params
{
    VkCommandBuffer commandBuffer;
    uint32_t stageCount;
    const VkShaderStageFlagBits *pStages;
    const VkShaderEXT *pShaders;
};

struct vkCmdBindShadingRateImageNV_params
{
    VkCommandBuffer commandBuffer;
    VkImageView DECLSPEC_ALIGN(8) imageView;
    VkImageLayout imageLayout;
};

struct vkCmdBindTileMemoryQCOM_params
{
    VkCommandBuffer commandBuffer;
    const VkTileMemoryBindInfoQCOM *pTileMemoryBindInfo;
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
    VkImage DECLSPEC_ALIGN(8) srcImage;
    VkImageLayout srcImageLayout;
    VkImage DECLSPEC_ALIGN(8) dstImage;
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
    VkBuffer DECLSPEC_ALIGN(8) instanceData;
    VkDeviceSize DECLSPEC_ALIGN(8) instanceOffset;
    VkBool32 update;
    VkAccelerationStructureNV DECLSPEC_ALIGN(8) dst;
    VkAccelerationStructureNV DECLSPEC_ALIGN(8) src;
    VkBuffer DECLSPEC_ALIGN(8) scratch;
    VkDeviceSize DECLSPEC_ALIGN(8) scratchOffset;
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

struct vkCmdBuildClusterAccelerationStructureIndirectNV_params
{
    VkCommandBuffer commandBuffer;
    const VkClusterAccelerationStructureCommandsInfoNV *pCommandInfos;
};

struct vkCmdBuildMicromapsEXT_params
{
    VkCommandBuffer commandBuffer;
    uint32_t infoCount;
    const VkMicromapBuildInfoEXT *pInfos;
};

struct vkCmdBuildPartitionedAccelerationStructuresNV_params
{
    VkCommandBuffer commandBuffer;
    const VkBuildPartitionedAccelerationStructureInfoNV *pBuildInfo;
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
    VkImage DECLSPEC_ALIGN(8) image;
    VkImageLayout imageLayout;
    const VkClearColorValue *pColor;
    uint32_t rangeCount;
    const VkImageSubresourceRange *pRanges;
};

struct vkCmdClearDepthStencilImage_params
{
    VkCommandBuffer commandBuffer;
    VkImage DECLSPEC_ALIGN(8) image;
    VkImageLayout imageLayout;
    const VkClearDepthStencilValue *pDepthStencil;
    uint32_t rangeCount;
    const VkImageSubresourceRange *pRanges;
};

struct vkCmdControlVideoCodingKHR_params
{
    VkCommandBuffer commandBuffer;
    const VkVideoCodingControlInfoKHR *pCodingControlInfo;
};

struct vkCmdConvertCooperativeVectorMatrixNV_params
{
    VkCommandBuffer commandBuffer;
    uint32_t infoCount;
    const VkConvertCooperativeVectorMatrixInfoNV *pInfos;
};

struct vkCmdCopyAccelerationStructureKHR_params
{
    VkCommandBuffer commandBuffer;
    const VkCopyAccelerationStructureInfoKHR *pInfo;
};

struct vkCmdCopyAccelerationStructureNV_params
{
    VkCommandBuffer commandBuffer;
    VkAccelerationStructureNV DECLSPEC_ALIGN(8) dst;
    VkAccelerationStructureNV DECLSPEC_ALIGN(8) src;
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
    VkBuffer DECLSPEC_ALIGN(8) srcBuffer;
    VkBuffer DECLSPEC_ALIGN(8) dstBuffer;
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
    VkBuffer DECLSPEC_ALIGN(8) srcBuffer;
    VkImage DECLSPEC_ALIGN(8) dstImage;
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
    VkImage DECLSPEC_ALIGN(8) srcImage;
    VkImageLayout srcImageLayout;
    VkImage DECLSPEC_ALIGN(8) dstImage;
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
    VkImage DECLSPEC_ALIGN(8) srcImage;
    VkImageLayout srcImageLayout;
    VkBuffer DECLSPEC_ALIGN(8) dstBuffer;
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

struct vkCmdCopyMemoryIndirectNV_params
{
    VkCommandBuffer commandBuffer;
    VkDeviceAddress DECLSPEC_ALIGN(8) copyBufferAddress;
    uint32_t copyCount;
    uint32_t stride;
};

struct vkCmdCopyMemoryToAccelerationStructureKHR_params
{
    VkCommandBuffer commandBuffer;
    const VkCopyMemoryToAccelerationStructureInfoKHR *pInfo;
};

struct vkCmdCopyMemoryToImageIndirectNV_params
{
    VkCommandBuffer commandBuffer;
    VkDeviceAddress DECLSPEC_ALIGN(8) copyBufferAddress;
    uint32_t copyCount;
    uint32_t stride;
    VkImage DECLSPEC_ALIGN(8) dstImage;
    VkImageLayout dstImageLayout;
    const VkImageSubresourceLayers *pImageSubresources;
};

struct vkCmdCopyMemoryToMicromapEXT_params
{
    VkCommandBuffer commandBuffer;
    const VkCopyMemoryToMicromapInfoEXT *pInfo;
};

struct vkCmdCopyMicromapEXT_params
{
    VkCommandBuffer commandBuffer;
    const VkCopyMicromapInfoEXT *pInfo;
};

struct vkCmdCopyMicromapToMemoryEXT_params
{
    VkCommandBuffer commandBuffer;
    const VkCopyMicromapToMemoryInfoEXT *pInfo;
};

struct vkCmdCopyQueryPoolResults_params
{
    VkCommandBuffer commandBuffer;
    VkQueryPool DECLSPEC_ALIGN(8) queryPool;
    uint32_t firstQuery;
    uint32_t queryCount;
    VkBuffer DECLSPEC_ALIGN(8) dstBuffer;
    VkDeviceSize DECLSPEC_ALIGN(8) dstOffset;
    VkDeviceSize DECLSPEC_ALIGN(8) stride;
    VkQueryResultFlags flags;
};

struct vkCmdCopyTensorARM_params
{
    VkCommandBuffer commandBuffer;
    const VkCopyTensorInfoARM *pCopyTensorInfo;
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

struct vkCmdDecodeVideoKHR_params
{
    VkCommandBuffer commandBuffer;
    const VkVideoDecodeInfoKHR *pDecodeInfo;
};

struct vkCmdDecompressMemoryIndirectCountNV_params
{
    VkCommandBuffer commandBuffer;
    VkDeviceAddress DECLSPEC_ALIGN(8) indirectCommandsAddress;
    VkDeviceAddress DECLSPEC_ALIGN(8) indirectCommandsCountAddress;
    uint32_t stride;
};

struct vkCmdDecompressMemoryNV_params
{
    VkCommandBuffer commandBuffer;
    uint32_t decompressRegionCount;
    const VkDecompressMemoryRegionNV *pDecompressMemoryRegions;
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

struct vkCmdDispatchDataGraphARM_params
{
    VkCommandBuffer commandBuffer;
    VkDataGraphPipelineSessionARM DECLSPEC_ALIGN(8) session;
    const VkDataGraphPipelineDispatchInfoARM *pInfo;
};

struct vkCmdDispatchIndirect_params
{
    VkCommandBuffer commandBuffer;
    VkBuffer DECLSPEC_ALIGN(8) buffer;
    VkDeviceSize DECLSPEC_ALIGN(8) offset;
};

struct vkCmdDispatchTileQCOM_params
{
    VkCommandBuffer commandBuffer;
    const VkDispatchTileInfoQCOM *pDispatchTileInfo;
};

struct vkCmdDraw_params
{
    VkCommandBuffer commandBuffer;
    uint32_t vertexCount;
    uint32_t instanceCount;
    uint32_t firstVertex;
    uint32_t firstInstance;
};

struct vkCmdDrawClusterHUAWEI_params
{
    VkCommandBuffer commandBuffer;
    uint32_t groupCountX;
    uint32_t groupCountY;
    uint32_t groupCountZ;
};

struct vkCmdDrawClusterIndirectHUAWEI_params
{
    VkCommandBuffer commandBuffer;
    VkBuffer DECLSPEC_ALIGN(8) buffer;
    VkDeviceSize DECLSPEC_ALIGN(8) offset;
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
    VkBuffer DECLSPEC_ALIGN(8) buffer;
    VkDeviceSize DECLSPEC_ALIGN(8) offset;
    uint32_t drawCount;
    uint32_t stride;
};

struct vkCmdDrawIndexedIndirectCount_params
{
    VkCommandBuffer commandBuffer;
    VkBuffer DECLSPEC_ALIGN(8) buffer;
    VkDeviceSize DECLSPEC_ALIGN(8) offset;
    VkBuffer DECLSPEC_ALIGN(8) countBuffer;
    VkDeviceSize DECLSPEC_ALIGN(8) countBufferOffset;
    uint32_t maxDrawCount;
    uint32_t stride;
};

struct vkCmdDrawIndexedIndirectCountAMD_params
{
    VkCommandBuffer commandBuffer;
    VkBuffer DECLSPEC_ALIGN(8) buffer;
    VkDeviceSize DECLSPEC_ALIGN(8) offset;
    VkBuffer DECLSPEC_ALIGN(8) countBuffer;
    VkDeviceSize DECLSPEC_ALIGN(8) countBufferOffset;
    uint32_t maxDrawCount;
    uint32_t stride;
};

struct vkCmdDrawIndexedIndirectCountKHR_params
{
    VkCommandBuffer commandBuffer;
    VkBuffer DECLSPEC_ALIGN(8) buffer;
    VkDeviceSize DECLSPEC_ALIGN(8) offset;
    VkBuffer DECLSPEC_ALIGN(8) countBuffer;
    VkDeviceSize DECLSPEC_ALIGN(8) countBufferOffset;
    uint32_t maxDrawCount;
    uint32_t stride;
};

struct vkCmdDrawIndirect_params
{
    VkCommandBuffer commandBuffer;
    VkBuffer DECLSPEC_ALIGN(8) buffer;
    VkDeviceSize DECLSPEC_ALIGN(8) offset;
    uint32_t drawCount;
    uint32_t stride;
};

struct vkCmdDrawIndirectByteCountEXT_params
{
    VkCommandBuffer commandBuffer;
    uint32_t instanceCount;
    uint32_t firstInstance;
    VkBuffer DECLSPEC_ALIGN(8) counterBuffer;
    VkDeviceSize DECLSPEC_ALIGN(8) counterBufferOffset;
    uint32_t counterOffset;
    uint32_t vertexStride;
};

struct vkCmdDrawIndirectCount_params
{
    VkCommandBuffer commandBuffer;
    VkBuffer DECLSPEC_ALIGN(8) buffer;
    VkDeviceSize DECLSPEC_ALIGN(8) offset;
    VkBuffer DECLSPEC_ALIGN(8) countBuffer;
    VkDeviceSize DECLSPEC_ALIGN(8) countBufferOffset;
    uint32_t maxDrawCount;
    uint32_t stride;
};

struct vkCmdDrawIndirectCountAMD_params
{
    VkCommandBuffer commandBuffer;
    VkBuffer DECLSPEC_ALIGN(8) buffer;
    VkDeviceSize DECLSPEC_ALIGN(8) offset;
    VkBuffer DECLSPEC_ALIGN(8) countBuffer;
    VkDeviceSize DECLSPEC_ALIGN(8) countBufferOffset;
    uint32_t maxDrawCount;
    uint32_t stride;
};

struct vkCmdDrawIndirectCountKHR_params
{
    VkCommandBuffer commandBuffer;
    VkBuffer DECLSPEC_ALIGN(8) buffer;
    VkDeviceSize DECLSPEC_ALIGN(8) offset;
    VkBuffer DECLSPEC_ALIGN(8) countBuffer;
    VkDeviceSize DECLSPEC_ALIGN(8) countBufferOffset;
    uint32_t maxDrawCount;
    uint32_t stride;
};

struct vkCmdDrawMeshTasksEXT_params
{
    VkCommandBuffer commandBuffer;
    uint32_t groupCountX;
    uint32_t groupCountY;
    uint32_t groupCountZ;
};

struct vkCmdDrawMeshTasksIndirectCountEXT_params
{
    VkCommandBuffer commandBuffer;
    VkBuffer DECLSPEC_ALIGN(8) buffer;
    VkDeviceSize DECLSPEC_ALIGN(8) offset;
    VkBuffer DECLSPEC_ALIGN(8) countBuffer;
    VkDeviceSize DECLSPEC_ALIGN(8) countBufferOffset;
    uint32_t maxDrawCount;
    uint32_t stride;
};

struct vkCmdDrawMeshTasksIndirectCountNV_params
{
    VkCommandBuffer commandBuffer;
    VkBuffer DECLSPEC_ALIGN(8) buffer;
    VkDeviceSize DECLSPEC_ALIGN(8) offset;
    VkBuffer DECLSPEC_ALIGN(8) countBuffer;
    VkDeviceSize DECLSPEC_ALIGN(8) countBufferOffset;
    uint32_t maxDrawCount;
    uint32_t stride;
};

struct vkCmdDrawMeshTasksIndirectEXT_params
{
    VkCommandBuffer commandBuffer;
    VkBuffer DECLSPEC_ALIGN(8) buffer;
    VkDeviceSize DECLSPEC_ALIGN(8) offset;
    uint32_t drawCount;
    uint32_t stride;
};

struct vkCmdDrawMeshTasksIndirectNV_params
{
    VkCommandBuffer commandBuffer;
    VkBuffer DECLSPEC_ALIGN(8) buffer;
    VkDeviceSize DECLSPEC_ALIGN(8) offset;
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

struct vkCmdEncodeVideoKHR_params
{
    VkCommandBuffer commandBuffer;
    const VkVideoEncodeInfoKHR *pEncodeInfo;
};

struct vkCmdEndConditionalRenderingEXT_params
{
    VkCommandBuffer commandBuffer;
};

struct vkCmdEndDebugUtilsLabelEXT_params
{
    VkCommandBuffer commandBuffer;
};

struct vkCmdEndPerTileExecutionQCOM_params
{
    VkCommandBuffer commandBuffer;
    const VkPerTileEndInfoQCOM *pPerTileEndInfo;
};

struct vkCmdEndQuery_params
{
    VkCommandBuffer commandBuffer;
    VkQueryPool DECLSPEC_ALIGN(8) queryPool;
    uint32_t query;
};

struct vkCmdEndQueryIndexedEXT_params
{
    VkCommandBuffer commandBuffer;
    VkQueryPool DECLSPEC_ALIGN(8) queryPool;
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

struct vkCmdEndRendering2EXT_params
{
    VkCommandBuffer commandBuffer;
    const VkRenderingEndInfoEXT *pRenderingEndInfo;
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

struct vkCmdEndVideoCodingKHR_params
{
    VkCommandBuffer commandBuffer;
    const VkVideoEndCodingInfoKHR *pEndCodingInfo;
};

struct vkCmdExecuteCommands_params
{
    VkCommandBuffer commandBuffer;
    uint32_t commandBufferCount;
    const VkCommandBuffer *pCommandBuffers;
};

struct vkCmdExecuteGeneratedCommandsEXT_params
{
    VkCommandBuffer commandBuffer;
    VkBool32 isPreprocessed;
    const VkGeneratedCommandsInfoEXT *pGeneratedCommandsInfo;
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
    VkBuffer DECLSPEC_ALIGN(8) dstBuffer;
    VkDeviceSize DECLSPEC_ALIGN(8) dstOffset;
    VkDeviceSize DECLSPEC_ALIGN(8) size;
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

struct vkCmdOpticalFlowExecuteNV_params
{
    VkCommandBuffer commandBuffer;
    VkOpticalFlowSessionNV DECLSPEC_ALIGN(8) session;
    const VkOpticalFlowExecuteInfoNV *pExecuteInfo;
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

struct vkCmdPreprocessGeneratedCommandsEXT_params
{
    VkCommandBuffer commandBuffer;
    const VkGeneratedCommandsInfoEXT *pGeneratedCommandsInfo;
    VkCommandBuffer stateCommandBuffer;
};

struct vkCmdPreprocessGeneratedCommandsNV_params
{
    VkCommandBuffer commandBuffer;
    const VkGeneratedCommandsInfoNV *pGeneratedCommandsInfo;
};

struct vkCmdPushConstants_params
{
    VkCommandBuffer commandBuffer;
    VkPipelineLayout DECLSPEC_ALIGN(8) layout;
    VkShaderStageFlags stageFlags;
    uint32_t offset;
    uint32_t size;
    const void *pValues;
};

struct vkCmdPushConstants2_params
{
    VkCommandBuffer commandBuffer;
    const VkPushConstantsInfo *pPushConstantsInfo;
};

struct vkCmdPushConstants2KHR_params
{
    VkCommandBuffer commandBuffer;
    const VkPushConstantsInfo *pPushConstantsInfo;
};

struct vkCmdPushDescriptorSet_params
{
    VkCommandBuffer commandBuffer;
    VkPipelineBindPoint pipelineBindPoint;
    VkPipelineLayout DECLSPEC_ALIGN(8) layout;
    uint32_t set;
    uint32_t descriptorWriteCount;
    const VkWriteDescriptorSet *pDescriptorWrites;
};

struct vkCmdPushDescriptorSet2_params
{
    VkCommandBuffer commandBuffer;
    const VkPushDescriptorSetInfo *pPushDescriptorSetInfo;
};

struct vkCmdPushDescriptorSet2KHR_params
{
    VkCommandBuffer commandBuffer;
    const VkPushDescriptorSetInfo *pPushDescriptorSetInfo;
};

struct vkCmdPushDescriptorSetKHR_params
{
    VkCommandBuffer commandBuffer;
    VkPipelineBindPoint pipelineBindPoint;
    VkPipelineLayout DECLSPEC_ALIGN(8) layout;
    uint32_t set;
    uint32_t descriptorWriteCount;
    const VkWriteDescriptorSet *pDescriptorWrites;
};

struct vkCmdPushDescriptorSetWithTemplate_params
{
    VkCommandBuffer commandBuffer;
    VkDescriptorUpdateTemplate DECLSPEC_ALIGN(8) descriptorUpdateTemplate;
    VkPipelineLayout DECLSPEC_ALIGN(8) layout;
    uint32_t set;
    const void *pData;
};

struct vkCmdPushDescriptorSetWithTemplate2_params
{
    VkCommandBuffer commandBuffer;
    const VkPushDescriptorSetWithTemplateInfo *pPushDescriptorSetWithTemplateInfo;
};

struct vkCmdPushDescriptorSetWithTemplate2KHR_params
{
    VkCommandBuffer commandBuffer;
    const VkPushDescriptorSetWithTemplateInfo *pPushDescriptorSetWithTemplateInfo;
};

struct vkCmdPushDescriptorSetWithTemplateKHR_params
{
    VkCommandBuffer commandBuffer;
    VkDescriptorUpdateTemplate DECLSPEC_ALIGN(8) descriptorUpdateTemplate;
    VkPipelineLayout DECLSPEC_ALIGN(8) layout;
    uint32_t set;
    const void *pData;
};

struct vkCmdResetEvent_params
{
    VkCommandBuffer commandBuffer;
    VkEvent DECLSPEC_ALIGN(8) event;
    VkPipelineStageFlags stageMask;
};

struct vkCmdResetEvent2_params
{
    VkCommandBuffer commandBuffer;
    VkEvent DECLSPEC_ALIGN(8) event;
    VkPipelineStageFlags2 DECLSPEC_ALIGN(8) stageMask;
};

struct vkCmdResetEvent2KHR_params
{
    VkCommandBuffer commandBuffer;
    VkEvent DECLSPEC_ALIGN(8) event;
    VkPipelineStageFlags2 DECLSPEC_ALIGN(8) stageMask;
};

struct vkCmdResetQueryPool_params
{
    VkCommandBuffer commandBuffer;
    VkQueryPool DECLSPEC_ALIGN(8) queryPool;
    uint32_t firstQuery;
    uint32_t queryCount;
};

struct vkCmdResolveImage_params
{
    VkCommandBuffer commandBuffer;
    VkImage DECLSPEC_ALIGN(8) srcImage;
    VkImageLayout srcImageLayout;
    VkImage DECLSPEC_ALIGN(8) dstImage;
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

struct vkCmdSetAlphaToCoverageEnableEXT_params
{
    VkCommandBuffer commandBuffer;
    VkBool32 alphaToCoverageEnable;
};

struct vkCmdSetAlphaToOneEnableEXT_params
{
    VkCommandBuffer commandBuffer;
    VkBool32 alphaToOneEnable;
};

struct vkCmdSetAttachmentFeedbackLoopEnableEXT_params
{
    VkCommandBuffer commandBuffer;
    VkImageAspectFlags aspectMask;
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

struct vkCmdSetColorBlendAdvancedEXT_params
{
    VkCommandBuffer commandBuffer;
    uint32_t firstAttachment;
    uint32_t attachmentCount;
    const VkColorBlendAdvancedEXT *pColorBlendAdvanced;
};

struct vkCmdSetColorBlendEnableEXT_params
{
    VkCommandBuffer commandBuffer;
    uint32_t firstAttachment;
    uint32_t attachmentCount;
    const VkBool32 *pColorBlendEnables;
};

struct vkCmdSetColorBlendEquationEXT_params
{
    VkCommandBuffer commandBuffer;
    uint32_t firstAttachment;
    uint32_t attachmentCount;
    const VkColorBlendEquationEXT *pColorBlendEquations;
};

struct vkCmdSetColorWriteEnableEXT_params
{
    VkCommandBuffer commandBuffer;
    uint32_t attachmentCount;
    const VkBool32 *pColorWriteEnables;
};

struct vkCmdSetColorWriteMaskEXT_params
{
    VkCommandBuffer commandBuffer;
    uint32_t firstAttachment;
    uint32_t attachmentCount;
    const VkColorComponentFlags *pColorWriteMasks;
};

struct vkCmdSetConservativeRasterizationModeEXT_params
{
    VkCommandBuffer commandBuffer;
    VkConservativeRasterizationModeEXT conservativeRasterizationMode;
};

struct vkCmdSetCoverageModulationModeNV_params
{
    VkCommandBuffer commandBuffer;
    VkCoverageModulationModeNV coverageModulationMode;
};

struct vkCmdSetCoverageModulationTableEnableNV_params
{
    VkCommandBuffer commandBuffer;
    VkBool32 coverageModulationTableEnable;
};

struct vkCmdSetCoverageModulationTableNV_params
{
    VkCommandBuffer commandBuffer;
    uint32_t coverageModulationTableCount;
    const float *pCoverageModulationTable;
};

struct vkCmdSetCoverageReductionModeNV_params
{
    VkCommandBuffer commandBuffer;
    VkCoverageReductionModeNV coverageReductionMode;
};

struct vkCmdSetCoverageToColorEnableNV_params
{
    VkCommandBuffer commandBuffer;
    VkBool32 coverageToColorEnable;
};

struct vkCmdSetCoverageToColorLocationNV_params
{
    VkCommandBuffer commandBuffer;
    uint32_t coverageToColorLocation;
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

struct vkCmdSetDepthBias2EXT_params
{
    VkCommandBuffer commandBuffer;
    const VkDepthBiasInfoEXT *pDepthBiasInfo;
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

struct vkCmdSetDepthClampEnableEXT_params
{
    VkCommandBuffer commandBuffer;
    VkBool32 depthClampEnable;
};

struct vkCmdSetDepthClampRangeEXT_params
{
    VkCommandBuffer commandBuffer;
    VkDepthClampModeEXT depthClampMode;
    const VkDepthClampRangeEXT *pDepthClampRange;
};

struct vkCmdSetDepthClipEnableEXT_params
{
    VkCommandBuffer commandBuffer;
    VkBool32 depthClipEnable;
};

struct vkCmdSetDepthClipNegativeOneToOneEXT_params
{
    VkCommandBuffer commandBuffer;
    VkBool32 negativeOneToOne;
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

struct vkCmdSetDescriptorBufferOffsets2EXT_params
{
    VkCommandBuffer commandBuffer;
    const VkSetDescriptorBufferOffsetsInfoEXT *pSetDescriptorBufferOffsetsInfo;
};

struct vkCmdSetDescriptorBufferOffsetsEXT_params
{
    VkCommandBuffer commandBuffer;
    VkPipelineBindPoint pipelineBindPoint;
    VkPipelineLayout DECLSPEC_ALIGN(8) layout;
    uint32_t firstSet;
    uint32_t setCount;
    const uint32_t *pBufferIndices;
    const VkDeviceSize *pOffsets;
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

struct vkCmdSetDiscardRectangleEnableEXT_params
{
    VkCommandBuffer commandBuffer;
    VkBool32 discardRectangleEnable;
};

struct vkCmdSetDiscardRectangleModeEXT_params
{
    VkCommandBuffer commandBuffer;
    VkDiscardRectangleModeEXT discardRectangleMode;
};

struct vkCmdSetEvent_params
{
    VkCommandBuffer commandBuffer;
    VkEvent DECLSPEC_ALIGN(8) event;
    VkPipelineStageFlags stageMask;
};

struct vkCmdSetEvent2_params
{
    VkCommandBuffer commandBuffer;
    VkEvent DECLSPEC_ALIGN(8) event;
    const VkDependencyInfo *pDependencyInfo;
};

struct vkCmdSetEvent2KHR_params
{
    VkCommandBuffer commandBuffer;
    VkEvent DECLSPEC_ALIGN(8) event;
    const VkDependencyInfo *pDependencyInfo;
};

struct vkCmdSetExclusiveScissorEnableNV_params
{
    VkCommandBuffer commandBuffer;
    uint32_t firstExclusiveScissor;
    uint32_t exclusiveScissorCount;
    const VkBool32 *pExclusiveScissorEnables;
};

struct vkCmdSetExclusiveScissorNV_params
{
    VkCommandBuffer commandBuffer;
    uint32_t firstExclusiveScissor;
    uint32_t exclusiveScissorCount;
    const VkRect2D *pExclusiveScissors;
};

struct vkCmdSetExtraPrimitiveOverestimationSizeEXT_params
{
    VkCommandBuffer commandBuffer;
    float extraPrimitiveOverestimationSize;
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

struct vkCmdSetLineRasterizationModeEXT_params
{
    VkCommandBuffer commandBuffer;
    VkLineRasterizationModeEXT lineRasterizationMode;
};

struct vkCmdSetLineStipple_params
{
    VkCommandBuffer commandBuffer;
    uint32_t lineStippleFactor;
    uint16_t lineStipplePattern;
};

struct vkCmdSetLineStippleEXT_params
{
    VkCommandBuffer commandBuffer;
    uint32_t lineStippleFactor;
    uint16_t lineStipplePattern;
};

struct vkCmdSetLineStippleEnableEXT_params
{
    VkCommandBuffer commandBuffer;
    VkBool32 stippledLineEnable;
};

struct vkCmdSetLineStippleKHR_params
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

struct vkCmdSetLogicOpEnableEXT_params
{
    VkCommandBuffer commandBuffer;
    VkBool32 logicOpEnable;
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
    VkResult result;
};

struct vkCmdSetPerformanceOverrideINTEL_params
{
    VkCommandBuffer commandBuffer;
    const VkPerformanceOverrideInfoINTEL *pOverrideInfo;
    VkResult result;
};

struct vkCmdSetPerformanceStreamMarkerINTEL_params
{
    VkCommandBuffer commandBuffer;
    const VkPerformanceStreamMarkerInfoINTEL *pMarkerInfo;
    VkResult result;
};

struct vkCmdSetPolygonModeEXT_params
{
    VkCommandBuffer commandBuffer;
    VkPolygonMode polygonMode;
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

struct vkCmdSetProvokingVertexModeEXT_params
{
    VkCommandBuffer commandBuffer;
    VkProvokingVertexModeEXT provokingVertexMode;
};

struct vkCmdSetRasterizationSamplesEXT_params
{
    VkCommandBuffer commandBuffer;
    VkSampleCountFlagBits rasterizationSamples;
};

struct vkCmdSetRasterizationStreamEXT_params
{
    VkCommandBuffer commandBuffer;
    uint32_t rasterizationStream;
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

struct vkCmdSetRenderingAttachmentLocations_params
{
    VkCommandBuffer commandBuffer;
    const VkRenderingAttachmentLocationInfo *pLocationInfo;
};

struct vkCmdSetRenderingAttachmentLocationsKHR_params
{
    VkCommandBuffer commandBuffer;
    const VkRenderingAttachmentLocationInfo *pLocationInfo;
};

struct vkCmdSetRenderingInputAttachmentIndices_params
{
    VkCommandBuffer commandBuffer;
    const VkRenderingInputAttachmentIndexInfo *pInputAttachmentIndexInfo;
};

struct vkCmdSetRenderingInputAttachmentIndicesKHR_params
{
    VkCommandBuffer commandBuffer;
    const VkRenderingInputAttachmentIndexInfo *pInputAttachmentIndexInfo;
};

struct vkCmdSetRepresentativeFragmentTestEnableNV_params
{
    VkCommandBuffer commandBuffer;
    VkBool32 representativeFragmentTestEnable;
};

struct vkCmdSetSampleLocationsEXT_params
{
    VkCommandBuffer commandBuffer;
    const VkSampleLocationsInfoEXT *pSampleLocationsInfo;
};

struct vkCmdSetSampleLocationsEnableEXT_params
{
    VkCommandBuffer commandBuffer;
    VkBool32 sampleLocationsEnable;
};

struct vkCmdSetSampleMaskEXT_params
{
    VkCommandBuffer commandBuffer;
    VkSampleCountFlagBits samples;
    const VkSampleMask *pSampleMask;
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

struct vkCmdSetShadingRateImageEnableNV_params
{
    VkCommandBuffer commandBuffer;
    VkBool32 shadingRateImageEnable;
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

struct vkCmdSetTessellationDomainOriginEXT_params
{
    VkCommandBuffer commandBuffer;
    VkTessellationDomainOrigin domainOrigin;
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

struct vkCmdSetViewportSwizzleNV_params
{
    VkCommandBuffer commandBuffer;
    uint32_t firstViewport;
    uint32_t viewportCount;
    const VkViewportSwizzleNV *pViewportSwizzles;
};

struct vkCmdSetViewportWScalingEnableNV_params
{
    VkCommandBuffer commandBuffer;
    VkBool32 viewportWScalingEnable;
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
    VkDeviceAddress DECLSPEC_ALIGN(8) indirectDeviceAddress;
};

struct vkCmdTraceRaysIndirectKHR_params
{
    VkCommandBuffer commandBuffer;
    const VkStridedDeviceAddressRegionKHR *pRaygenShaderBindingTable;
    const VkStridedDeviceAddressRegionKHR *pMissShaderBindingTable;
    const VkStridedDeviceAddressRegionKHR *pHitShaderBindingTable;
    const VkStridedDeviceAddressRegionKHR *pCallableShaderBindingTable;
    VkDeviceAddress DECLSPEC_ALIGN(8) indirectDeviceAddress;
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
    VkBuffer DECLSPEC_ALIGN(8) raygenShaderBindingTableBuffer;
    VkDeviceSize DECLSPEC_ALIGN(8) raygenShaderBindingOffset;
    VkBuffer DECLSPEC_ALIGN(8) missShaderBindingTableBuffer;
    VkDeviceSize DECLSPEC_ALIGN(8) missShaderBindingOffset;
    VkDeviceSize DECLSPEC_ALIGN(8) missShaderBindingStride;
    VkBuffer DECLSPEC_ALIGN(8) hitShaderBindingTableBuffer;
    VkDeviceSize DECLSPEC_ALIGN(8) hitShaderBindingOffset;
    VkDeviceSize DECLSPEC_ALIGN(8) hitShaderBindingStride;
    VkBuffer DECLSPEC_ALIGN(8) callableShaderBindingTableBuffer;
    VkDeviceSize DECLSPEC_ALIGN(8) callableShaderBindingOffset;
    VkDeviceSize DECLSPEC_ALIGN(8) callableShaderBindingStride;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
};

struct vkCmdUpdateBuffer_params
{
    VkCommandBuffer commandBuffer;
    VkBuffer DECLSPEC_ALIGN(8) dstBuffer;
    VkDeviceSize DECLSPEC_ALIGN(8) dstOffset;
    VkDeviceSize DECLSPEC_ALIGN(8) dataSize;
    const void *pData;
};

struct vkCmdUpdatePipelineIndirectBufferNV_params
{
    VkCommandBuffer commandBuffer;
    VkPipelineBindPoint pipelineBindPoint;
    VkPipeline DECLSPEC_ALIGN(8) pipeline;
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
    VkQueryPool DECLSPEC_ALIGN(8) queryPool;
    uint32_t firstQuery;
};

struct vkCmdWriteAccelerationStructuresPropertiesNV_params
{
    VkCommandBuffer commandBuffer;
    uint32_t accelerationStructureCount;
    const VkAccelerationStructureNV *pAccelerationStructures;
    VkQueryType queryType;
    VkQueryPool DECLSPEC_ALIGN(8) queryPool;
    uint32_t firstQuery;
};

struct vkCmdWriteBufferMarker2AMD_params
{
    VkCommandBuffer commandBuffer;
    VkPipelineStageFlags2 DECLSPEC_ALIGN(8) stage;
    VkBuffer DECLSPEC_ALIGN(8) dstBuffer;
    VkDeviceSize DECLSPEC_ALIGN(8) dstOffset;
    uint32_t marker;
};

struct vkCmdWriteBufferMarkerAMD_params
{
    VkCommandBuffer commandBuffer;
    VkPipelineStageFlagBits pipelineStage;
    VkBuffer DECLSPEC_ALIGN(8) dstBuffer;
    VkDeviceSize DECLSPEC_ALIGN(8) dstOffset;
    uint32_t marker;
};

struct vkCmdWriteMicromapsPropertiesEXT_params
{
    VkCommandBuffer commandBuffer;
    uint32_t micromapCount;
    const VkMicromapEXT *pMicromaps;
    VkQueryType queryType;
    VkQueryPool DECLSPEC_ALIGN(8) queryPool;
    uint32_t firstQuery;
};

struct vkCmdWriteTimestamp_params
{
    VkCommandBuffer commandBuffer;
    VkPipelineStageFlagBits pipelineStage;
    VkQueryPool DECLSPEC_ALIGN(8) queryPool;
    uint32_t query;
};

struct vkCmdWriteTimestamp2_params
{
    VkCommandBuffer commandBuffer;
    VkPipelineStageFlags2 DECLSPEC_ALIGN(8) stage;
    VkQueryPool DECLSPEC_ALIGN(8) queryPool;
    uint32_t query;
};

struct vkCmdWriteTimestamp2KHR_params
{
    VkCommandBuffer commandBuffer;
    VkPipelineStageFlags2 DECLSPEC_ALIGN(8) stage;
    VkQueryPool DECLSPEC_ALIGN(8) queryPool;
    uint32_t query;
};

struct vkCompileDeferredNV_params
{
    VkDevice device;
    VkPipeline DECLSPEC_ALIGN(8) pipeline;
    uint32_t shader;
    VkResult result;
};

struct vkConvertCooperativeVectorMatrixNV_params
{
    VkDevice device;
    const VkConvertCooperativeVectorMatrixInfoNV *pInfo;
    VkResult result;
};

struct vkCopyAccelerationStructureKHR_params
{
    VkDevice device;
    VkDeferredOperationKHR DECLSPEC_ALIGN(8) deferredOperation;
    const VkCopyAccelerationStructureInfoKHR *pInfo;
    VkResult result;
};

struct vkCopyAccelerationStructureToMemoryKHR_params
{
    VkDevice device;
    VkDeferredOperationKHR DECLSPEC_ALIGN(8) deferredOperation;
    const VkCopyAccelerationStructureToMemoryInfoKHR *pInfo;
    VkResult result;
};

struct vkCopyImageToImage_params
{
    VkDevice device;
    const VkCopyImageToImageInfo *pCopyImageToImageInfo;
    VkResult result;
};

struct vkCopyImageToImageEXT_params
{
    VkDevice device;
    const VkCopyImageToImageInfo *pCopyImageToImageInfo;
    VkResult result;
};

struct vkCopyImageToMemory_params
{
    VkDevice device;
    const VkCopyImageToMemoryInfo *pCopyImageToMemoryInfo;
    VkResult result;
};

struct vkCopyImageToMemoryEXT_params
{
    VkDevice device;
    const VkCopyImageToMemoryInfo *pCopyImageToMemoryInfo;
    VkResult result;
};

struct vkCopyMemoryToAccelerationStructureKHR_params
{
    VkDevice device;
    VkDeferredOperationKHR DECLSPEC_ALIGN(8) deferredOperation;
    const VkCopyMemoryToAccelerationStructureInfoKHR *pInfo;
    VkResult result;
};

struct vkCopyMemoryToImage_params
{
    VkDevice device;
    const VkCopyMemoryToImageInfo *pCopyMemoryToImageInfo;
    VkResult result;
};

struct vkCopyMemoryToImageEXT_params
{
    VkDevice device;
    const VkCopyMemoryToImageInfo *pCopyMemoryToImageInfo;
    VkResult result;
};

struct vkCopyMemoryToMicromapEXT_params
{
    VkDevice device;
    VkDeferredOperationKHR DECLSPEC_ALIGN(8) deferredOperation;
    const VkCopyMemoryToMicromapInfoEXT *pInfo;
    VkResult result;
};

struct vkCopyMicromapEXT_params
{
    VkDevice device;
    VkDeferredOperationKHR DECLSPEC_ALIGN(8) deferredOperation;
    const VkCopyMicromapInfoEXT *pInfo;
    VkResult result;
};

struct vkCopyMicromapToMemoryEXT_params
{
    VkDevice device;
    VkDeferredOperationKHR DECLSPEC_ALIGN(8) deferredOperation;
    const VkCopyMicromapToMemoryInfoEXT *pInfo;
    VkResult result;
};

struct vkCreateAccelerationStructureKHR_params
{
    VkDevice device;
    const VkAccelerationStructureCreateInfoKHR *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkAccelerationStructureKHR *pAccelerationStructure;
    VkResult result;
};

struct vkCreateAccelerationStructureNV_params
{
    VkDevice device;
    const VkAccelerationStructureCreateInfoNV *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkAccelerationStructureNV *pAccelerationStructure;
    VkResult result;
};

struct vkCreateBuffer_params
{
    VkDevice device;
    const VkBufferCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkBuffer *pBuffer;
    VkResult result;
};

struct vkCreateBufferView_params
{
    VkDevice device;
    const VkBufferViewCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkBufferView *pView;
    VkResult result;
};

struct vkCreateCommandPool_params
{
    VkDevice device;
    const VkCommandPoolCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkCommandPool *pCommandPool;
    void *client_ptr;
    VkResult result;
};

struct vkCreateComputePipelines_params
{
    VkDevice device;
    VkPipelineCache DECLSPEC_ALIGN(8) pipelineCache;
    uint32_t createInfoCount;
    const VkComputePipelineCreateInfo *pCreateInfos;
    const VkAllocationCallbacks *pAllocator;
    VkPipeline *pPipelines;
    VkResult result;
};

struct vkCreateCuFunctionNVX_params
{
    VkDevice device;
    const VkCuFunctionCreateInfoNVX *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkCuFunctionNVX *pFunction;
    VkResult result;
};

struct vkCreateCuModuleNVX_params
{
    VkDevice device;
    const VkCuModuleCreateInfoNVX *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkCuModuleNVX *pModule;
    VkResult result;
};

struct vkCreateDataGraphPipelineSessionARM_params
{
    VkDevice device;
    const VkDataGraphPipelineSessionCreateInfoARM *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkDataGraphPipelineSessionARM *pSession;
    VkResult result;
};

struct vkCreateDataGraphPipelinesARM_params
{
    VkDevice device;
    VkDeferredOperationKHR DECLSPEC_ALIGN(8) deferredOperation;
    VkPipelineCache DECLSPEC_ALIGN(8) pipelineCache;
    uint32_t createInfoCount;
    const VkDataGraphPipelineCreateInfoARM *pCreateInfos;
    const VkAllocationCallbacks *pAllocator;
    VkPipeline *pPipelines;
    VkResult result;
};

struct vkCreateDebugReportCallbackEXT_params
{
    VkInstance instance;
    const VkDebugReportCallbackCreateInfoEXT *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkDebugReportCallbackEXT *pCallback;
    VkResult result;
};

struct vkCreateDebugUtilsMessengerEXT_params
{
    VkInstance instance;
    const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkDebugUtilsMessengerEXT *pMessenger;
    VkResult result;
};

struct vkCreateDeferredOperationKHR_params
{
    VkDevice device;
    const VkAllocationCallbacks *pAllocator;
    VkDeferredOperationKHR *pDeferredOperation;
    VkResult result;
};

struct vkCreateDescriptorPool_params
{
    VkDevice device;
    const VkDescriptorPoolCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkDescriptorPool *pDescriptorPool;
    VkResult result;
};

struct vkCreateDescriptorSetLayout_params
{
    VkDevice device;
    const VkDescriptorSetLayoutCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkDescriptorSetLayout *pSetLayout;
    VkResult result;
};

struct vkCreateDescriptorUpdateTemplate_params
{
    VkDevice device;
    const VkDescriptorUpdateTemplateCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkDescriptorUpdateTemplate *pDescriptorUpdateTemplate;
    VkResult result;
};

struct vkCreateDescriptorUpdateTemplateKHR_params
{
    VkDevice device;
    const VkDescriptorUpdateTemplateCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkDescriptorUpdateTemplate *pDescriptorUpdateTemplate;
    VkResult result;
};

struct vkCreateDevice_params
{
    VkPhysicalDevice physicalDevice;
    const VkDeviceCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkDevice *pDevice;
    void *client_ptr;
    VkResult result;
};

struct vkCreateEvent_params
{
    VkDevice device;
    const VkEventCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkEvent *pEvent;
    VkResult result;
};

struct vkCreateFence_params
{
    VkDevice device;
    const VkFenceCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkFence *pFence;
    VkResult result;
};

struct vkCreateFramebuffer_params
{
    VkDevice device;
    const VkFramebufferCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkFramebuffer *pFramebuffer;
    VkResult result;
};

struct vkCreateGraphicsPipelines_params
{
    VkDevice device;
    VkPipelineCache DECLSPEC_ALIGN(8) pipelineCache;
    uint32_t createInfoCount;
    const VkGraphicsPipelineCreateInfo *pCreateInfos;
    const VkAllocationCallbacks *pAllocator;
    VkPipeline *pPipelines;
    VkResult result;
};

struct vkCreateImage_params
{
    VkDevice device;
    const VkImageCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkImage *pImage;
    VkResult result;
};

struct vkCreateImageView_params
{
    VkDevice device;
    const VkImageViewCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkImageView *pView;
    VkResult result;
};

struct vkCreateIndirectCommandsLayoutEXT_params
{
    VkDevice device;
    const VkIndirectCommandsLayoutCreateInfoEXT *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkIndirectCommandsLayoutEXT *pIndirectCommandsLayout;
    VkResult result;
};

struct vkCreateIndirectCommandsLayoutNV_params
{
    VkDevice device;
    const VkIndirectCommandsLayoutCreateInfoNV *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkIndirectCommandsLayoutNV *pIndirectCommandsLayout;
    VkResult result;
};

struct vkCreateIndirectExecutionSetEXT_params
{
    VkDevice device;
    const VkIndirectExecutionSetCreateInfoEXT *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkIndirectExecutionSetEXT *pIndirectExecutionSet;
    VkResult result;
};

struct vkCreateInstance_params
{
    const VkInstanceCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkInstance *pInstance;
    void *client_ptr;
    VkResult result;
};

struct vkCreateMicromapEXT_params
{
    VkDevice device;
    const VkMicromapCreateInfoEXT *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkMicromapEXT *pMicromap;
    VkResult result;
};

struct vkCreateOpticalFlowSessionNV_params
{
    VkDevice device;
    const VkOpticalFlowSessionCreateInfoNV *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkOpticalFlowSessionNV *pSession;
    VkResult result;
};

struct vkCreatePipelineBinariesKHR_params
{
    VkDevice device;
    const VkPipelineBinaryCreateInfoKHR *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkPipelineBinaryHandlesInfoKHR *pBinaries;
    VkResult result;
};

struct vkCreatePipelineCache_params
{
    VkDevice device;
    const VkPipelineCacheCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkPipelineCache *pPipelineCache;
    VkResult result;
};

struct vkCreatePipelineLayout_params
{
    VkDevice device;
    const VkPipelineLayoutCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkPipelineLayout *pPipelineLayout;
    VkResult result;
};

struct vkCreatePrivateDataSlot_params
{
    VkDevice device;
    const VkPrivateDataSlotCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkPrivateDataSlot *pPrivateDataSlot;
    VkResult result;
};

struct vkCreatePrivateDataSlotEXT_params
{
    VkDevice device;
    const VkPrivateDataSlotCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkPrivateDataSlot *pPrivateDataSlot;
    VkResult result;
};

struct vkCreateQueryPool_params
{
    VkDevice device;
    const VkQueryPoolCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkQueryPool *pQueryPool;
    VkResult result;
};

struct vkCreateRayTracingPipelinesKHR_params
{
    VkDevice device;
    VkDeferredOperationKHR DECLSPEC_ALIGN(8) deferredOperation;
    VkPipelineCache DECLSPEC_ALIGN(8) pipelineCache;
    uint32_t createInfoCount;
    const VkRayTracingPipelineCreateInfoKHR *pCreateInfos;
    const VkAllocationCallbacks *pAllocator;
    VkPipeline *pPipelines;
    VkResult result;
};

struct vkCreateRayTracingPipelinesNV_params
{
    VkDevice device;
    VkPipelineCache DECLSPEC_ALIGN(8) pipelineCache;
    uint32_t createInfoCount;
    const VkRayTracingPipelineCreateInfoNV *pCreateInfos;
    const VkAllocationCallbacks *pAllocator;
    VkPipeline *pPipelines;
    VkResult result;
};

struct vkCreateRenderPass_params
{
    VkDevice device;
    const VkRenderPassCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkRenderPass *pRenderPass;
    VkResult result;
};

struct vkCreateRenderPass2_params
{
    VkDevice device;
    const VkRenderPassCreateInfo2 *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkRenderPass *pRenderPass;
    VkResult result;
};

struct vkCreateRenderPass2KHR_params
{
    VkDevice device;
    const VkRenderPassCreateInfo2 *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkRenderPass *pRenderPass;
    VkResult result;
};

struct vkCreateSampler_params
{
    VkDevice device;
    const VkSamplerCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkSampler *pSampler;
    VkResult result;
};

struct vkCreateSamplerYcbcrConversion_params
{
    VkDevice device;
    const VkSamplerYcbcrConversionCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkSamplerYcbcrConversion *pYcbcrConversion;
    VkResult result;
};

struct vkCreateSamplerYcbcrConversionKHR_params
{
    VkDevice device;
    const VkSamplerYcbcrConversionCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkSamplerYcbcrConversion *pYcbcrConversion;
    VkResult result;
};

struct vkCreateSemaphore_params
{
    VkDevice device;
    const VkSemaphoreCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkSemaphore *pSemaphore;
    VkResult result;
};

struct vkCreateShaderModule_params
{
    VkDevice device;
    const VkShaderModuleCreateInfo *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkShaderModule *pShaderModule;
    VkResult result;
};

struct vkCreateShadersEXT_params
{
    VkDevice device;
    uint32_t createInfoCount;
    const VkShaderCreateInfoEXT *pCreateInfos;
    const VkAllocationCallbacks *pAllocator;
    VkShaderEXT *pShaders;
    VkResult result;
};

struct vkCreateSwapchainKHR_params
{
    VkDevice device;
    const VkSwapchainCreateInfoKHR *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkSwapchainKHR *pSwapchain;
    VkResult result;
};

struct vkCreateTensorARM_params
{
    VkDevice device;
    const VkTensorCreateInfoARM *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkTensorARM *pTensor;
    VkResult result;
};

struct vkCreateTensorViewARM_params
{
    VkDevice device;
    const VkTensorViewCreateInfoARM *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkTensorViewARM *pView;
    VkResult result;
};

struct vkCreateValidationCacheEXT_params
{
    VkDevice device;
    const VkValidationCacheCreateInfoEXT *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkValidationCacheEXT *pValidationCache;
    VkResult result;
};

struct vkCreateVideoSessionKHR_params
{
    VkDevice device;
    const VkVideoSessionCreateInfoKHR *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkVideoSessionKHR *pVideoSession;
    VkResult result;
};

struct vkCreateVideoSessionParametersKHR_params
{
    VkDevice device;
    const VkVideoSessionParametersCreateInfoKHR *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkVideoSessionParametersKHR *pVideoSessionParameters;
    VkResult result;
};

struct vkCreateWin32SurfaceKHR_params
{
    VkInstance instance;
    const VkWin32SurfaceCreateInfoKHR *pCreateInfo;
    const VkAllocationCallbacks *pAllocator;
    VkSurfaceKHR *pSurface;
    VkResult result;
};

struct vkDebugMarkerSetObjectNameEXT_params
{
    VkDevice device;
    const VkDebugMarkerObjectNameInfoEXT *pNameInfo;
    VkResult result;
};

struct vkDebugMarkerSetObjectTagEXT_params
{
    VkDevice device;
    const VkDebugMarkerObjectTagInfoEXT *pTagInfo;
    VkResult result;
};

struct vkDebugReportMessageEXT_params
{
    VkInstance instance;
    VkDebugReportFlagsEXT flags;
    VkDebugReportObjectTypeEXT objectType;
    uint64_t DECLSPEC_ALIGN(8) object;
    size_t location;
    int32_t messageCode;
    const char *pLayerPrefix;
    const char *pMessage;
};

struct vkDeferredOperationJoinKHR_params
{
    VkDevice device;
    VkDeferredOperationKHR DECLSPEC_ALIGN(8) operation;
    VkResult result;
};

struct vkDestroyAccelerationStructureKHR_params
{
    VkDevice device;
    VkAccelerationStructureKHR DECLSPEC_ALIGN(8) accelerationStructure;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyAccelerationStructureNV_params
{
    VkDevice device;
    VkAccelerationStructureNV DECLSPEC_ALIGN(8) accelerationStructure;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyBuffer_params
{
    VkDevice device;
    VkBuffer DECLSPEC_ALIGN(8) buffer;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyBufferView_params
{
    VkDevice device;
    VkBufferView DECLSPEC_ALIGN(8) bufferView;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyCommandPool_params
{
    VkDevice device;
    VkCommandPool DECLSPEC_ALIGN(8) commandPool;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyCuFunctionNVX_params
{
    VkDevice device;
    VkCuFunctionNVX DECLSPEC_ALIGN(8) function;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyCuModuleNVX_params
{
    VkDevice device;
    VkCuModuleNVX DECLSPEC_ALIGN(8) module;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyDataGraphPipelineSessionARM_params
{
    VkDevice device;
    VkDataGraphPipelineSessionARM DECLSPEC_ALIGN(8) session;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyDebugReportCallbackEXT_params
{
    VkInstance instance;
    VkDebugReportCallbackEXT DECLSPEC_ALIGN(8) callback;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyDebugUtilsMessengerEXT_params
{
    VkInstance instance;
    VkDebugUtilsMessengerEXT DECLSPEC_ALIGN(8) messenger;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyDeferredOperationKHR_params
{
    VkDevice device;
    VkDeferredOperationKHR DECLSPEC_ALIGN(8) operation;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyDescriptorPool_params
{
    VkDevice device;
    VkDescriptorPool DECLSPEC_ALIGN(8) descriptorPool;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyDescriptorSetLayout_params
{
    VkDevice device;
    VkDescriptorSetLayout DECLSPEC_ALIGN(8) descriptorSetLayout;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyDescriptorUpdateTemplate_params
{
    VkDevice device;
    VkDescriptorUpdateTemplate DECLSPEC_ALIGN(8) descriptorUpdateTemplate;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyDescriptorUpdateTemplateKHR_params
{
    VkDevice device;
    VkDescriptorUpdateTemplate DECLSPEC_ALIGN(8) descriptorUpdateTemplate;
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
    VkEvent DECLSPEC_ALIGN(8) event;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyFence_params
{
    VkDevice device;
    VkFence DECLSPEC_ALIGN(8) fence;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyFramebuffer_params
{
    VkDevice device;
    VkFramebuffer DECLSPEC_ALIGN(8) framebuffer;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyImage_params
{
    VkDevice device;
    VkImage DECLSPEC_ALIGN(8) image;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyImageView_params
{
    VkDevice device;
    VkImageView DECLSPEC_ALIGN(8) imageView;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyIndirectCommandsLayoutEXT_params
{
    VkDevice device;
    VkIndirectCommandsLayoutEXT DECLSPEC_ALIGN(8) indirectCommandsLayout;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyIndirectCommandsLayoutNV_params
{
    VkDevice device;
    VkIndirectCommandsLayoutNV DECLSPEC_ALIGN(8) indirectCommandsLayout;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyIndirectExecutionSetEXT_params
{
    VkDevice device;
    VkIndirectExecutionSetEXT DECLSPEC_ALIGN(8) indirectExecutionSet;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyInstance_params
{
    VkInstance instance;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyMicromapEXT_params
{
    VkDevice device;
    VkMicromapEXT DECLSPEC_ALIGN(8) micromap;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyOpticalFlowSessionNV_params
{
    VkDevice device;
    VkOpticalFlowSessionNV DECLSPEC_ALIGN(8) session;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyPipeline_params
{
    VkDevice device;
    VkPipeline DECLSPEC_ALIGN(8) pipeline;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyPipelineBinaryKHR_params
{
    VkDevice device;
    VkPipelineBinaryKHR DECLSPEC_ALIGN(8) pipelineBinary;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyPipelineCache_params
{
    VkDevice device;
    VkPipelineCache DECLSPEC_ALIGN(8) pipelineCache;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyPipelineLayout_params
{
    VkDevice device;
    VkPipelineLayout DECLSPEC_ALIGN(8) pipelineLayout;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyPrivateDataSlot_params
{
    VkDevice device;
    VkPrivateDataSlot DECLSPEC_ALIGN(8) privateDataSlot;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyPrivateDataSlotEXT_params
{
    VkDevice device;
    VkPrivateDataSlot DECLSPEC_ALIGN(8) privateDataSlot;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyQueryPool_params
{
    VkDevice device;
    VkQueryPool DECLSPEC_ALIGN(8) queryPool;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyRenderPass_params
{
    VkDevice device;
    VkRenderPass DECLSPEC_ALIGN(8) renderPass;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroySampler_params
{
    VkDevice device;
    VkSampler DECLSPEC_ALIGN(8) sampler;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroySamplerYcbcrConversion_params
{
    VkDevice device;
    VkSamplerYcbcrConversion DECLSPEC_ALIGN(8) ycbcrConversion;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroySamplerYcbcrConversionKHR_params
{
    VkDevice device;
    VkSamplerYcbcrConversion DECLSPEC_ALIGN(8) ycbcrConversion;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroySemaphore_params
{
    VkDevice device;
    VkSemaphore DECLSPEC_ALIGN(8) semaphore;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyShaderEXT_params
{
    VkDevice device;
    VkShaderEXT DECLSPEC_ALIGN(8) shader;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyShaderModule_params
{
    VkDevice device;
    VkShaderModule DECLSPEC_ALIGN(8) shaderModule;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroySurfaceKHR_params
{
    VkInstance instance;
    VkSurfaceKHR DECLSPEC_ALIGN(8) surface;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroySwapchainKHR_params
{
    VkDevice device;
    VkSwapchainKHR DECLSPEC_ALIGN(8) swapchain;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyTensorARM_params
{
    VkDevice device;
    VkTensorARM DECLSPEC_ALIGN(8) tensor;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyTensorViewARM_params
{
    VkDevice device;
    VkTensorViewARM DECLSPEC_ALIGN(8) tensorView;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyValidationCacheEXT_params
{
    VkDevice device;
    VkValidationCacheEXT DECLSPEC_ALIGN(8) validationCache;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyVideoSessionKHR_params
{
    VkDevice device;
    VkVideoSessionKHR DECLSPEC_ALIGN(8) videoSession;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDestroyVideoSessionParametersKHR_params
{
    VkDevice device;
    VkVideoSessionParametersKHR DECLSPEC_ALIGN(8) videoSessionParameters;
    const VkAllocationCallbacks *pAllocator;
};

struct vkDeviceWaitIdle_params
{
    VkDevice device;
    VkResult result;
};

struct vkEndCommandBuffer_params
{
    VkCommandBuffer commandBuffer;
    VkResult result;
};

struct vkEnumerateDeviceExtensionProperties_params
{
    VkPhysicalDevice physicalDevice;
    const char *pLayerName;
    uint32_t *pPropertyCount;
    VkExtensionProperties *pProperties;
    VkResult result;
};

struct vkEnumerateDeviceLayerProperties_params
{
    VkPhysicalDevice physicalDevice;
    uint32_t *pPropertyCount;
    VkLayerProperties *pProperties;
    VkResult result;
};

struct vkEnumerateInstanceExtensionProperties_params
{
    const char *pLayerName;
    uint32_t *pPropertyCount;
    VkExtensionProperties *pProperties;
    VkResult result;
};

struct vkEnumerateInstanceVersion_params
{
    uint32_t *pApiVersion;
    VkResult result;
};

struct vkEnumeratePhysicalDeviceGroups_params
{
    VkInstance instance;
    uint32_t *pPhysicalDeviceGroupCount;
    VkPhysicalDeviceGroupProperties *pPhysicalDeviceGroupProperties;
    VkResult result;
};

struct vkEnumeratePhysicalDeviceGroupsKHR_params
{
    VkInstance instance;
    uint32_t *pPhysicalDeviceGroupCount;
    VkPhysicalDeviceGroupProperties *pPhysicalDeviceGroupProperties;
    VkResult result;
};

struct vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR_params
{
    VkPhysicalDevice physicalDevice;
    uint32_t queueFamilyIndex;
    uint32_t *pCounterCount;
    VkPerformanceCounterKHR *pCounters;
    VkPerformanceCounterDescriptionKHR *pCounterDescriptions;
    VkResult result;
};

struct vkEnumeratePhysicalDevices_params
{
    VkInstance instance;
    uint32_t *pPhysicalDeviceCount;
    VkPhysicalDevice *pPhysicalDevices;
    VkResult result;
};

struct vkFlushMappedMemoryRanges_params
{
    VkDevice device;
    uint32_t memoryRangeCount;
    const VkMappedMemoryRange *pMemoryRanges;
    VkResult result;
};

struct vkFreeCommandBuffers_params
{
    VkDevice device;
    VkCommandPool DECLSPEC_ALIGN(8) commandPool;
    uint32_t commandBufferCount;
    const VkCommandBuffer *pCommandBuffers;
};

struct vkFreeDescriptorSets_params
{
    VkDevice device;
    VkDescriptorPool DECLSPEC_ALIGN(8) descriptorPool;
    uint32_t descriptorSetCount;
    const VkDescriptorSet *pDescriptorSets;
    VkResult result;
};

struct vkFreeMemory_params
{
    VkDevice device;
    VkDeviceMemory DECLSPEC_ALIGN(8) memory;
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
    VkAccelerationStructureNV DECLSPEC_ALIGN(8) accelerationStructure;
    size_t dataSize;
    void *pData;
    VkResult result;
};

struct vkGetAccelerationStructureMemoryRequirementsNV_params
{
    VkDevice device;
    const VkAccelerationStructureMemoryRequirementsInfoNV *pInfo;
    VkMemoryRequirements2KHR *pMemoryRequirements;
};

struct vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT_params
{
    VkDevice device;
    const VkAccelerationStructureCaptureDescriptorDataInfoEXT *pInfo;
    void *pData;
    VkResult result;
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
    VkBuffer DECLSPEC_ALIGN(8) buffer;
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

struct vkGetBufferOpaqueCaptureDescriptorDataEXT_params
{
    VkDevice device;
    const VkBufferCaptureDescriptorDataInfoEXT *pInfo;
    void *pData;
    VkResult result;
};

struct vkGetCalibratedTimestampsEXT_params
{
    VkDevice device;
    uint32_t timestampCount;
    const VkCalibratedTimestampInfoKHR *pTimestampInfos;
    uint64_t *pTimestamps;
    uint64_t *pMaxDeviation;
    VkResult result;
};

struct vkGetCalibratedTimestampsKHR_params
{
    VkDevice device;
    uint32_t timestampCount;
    const VkCalibratedTimestampInfoKHR *pTimestampInfos;
    uint64_t *pTimestamps;
    uint64_t *pMaxDeviation;
    VkResult result;
};

struct vkGetClusterAccelerationStructureBuildSizesNV_params
{
    VkDevice device;
    const VkClusterAccelerationStructureInputInfoNV *pInfo;
    VkAccelerationStructureBuildSizesInfoKHR *pSizeInfo;
};

struct vkGetDataGraphPipelineAvailablePropertiesARM_params
{
    VkDevice device;
    const VkDataGraphPipelineInfoARM *pPipelineInfo;
    uint32_t *pPropertiesCount;
    VkDataGraphPipelinePropertyARM *pProperties;
    VkResult result;
};

struct vkGetDataGraphPipelinePropertiesARM_params
{
    VkDevice device;
    const VkDataGraphPipelineInfoARM *pPipelineInfo;
    uint32_t propertiesCount;
    VkDataGraphPipelinePropertyQueryResultARM *pProperties;
    VkResult result;
};

struct vkGetDataGraphPipelineSessionBindPointRequirementsARM_params
{
    VkDevice device;
    const VkDataGraphPipelineSessionBindPointRequirementsInfoARM *pInfo;
    uint32_t *pBindPointRequirementCount;
    VkDataGraphPipelineSessionBindPointRequirementARM *pBindPointRequirements;
    VkResult result;
};

struct vkGetDataGraphPipelineSessionMemoryRequirementsARM_params
{
    VkDevice device;
    const VkDataGraphPipelineSessionMemoryRequirementsInfoARM *pInfo;
    VkMemoryRequirements2 *pMemoryRequirements;
};

struct vkGetDeferredOperationMaxConcurrencyKHR_params
{
    VkDevice device;
    VkDeferredOperationKHR DECLSPEC_ALIGN(8) operation;
    uint32_t result;
};

struct vkGetDeferredOperationResultKHR_params
{
    VkDevice device;
    VkDeferredOperationKHR DECLSPEC_ALIGN(8) operation;
    VkResult result;
};

struct vkGetDescriptorEXT_params
{
    VkDevice device;
    const VkDescriptorGetInfoEXT *pDescriptorInfo;
    size_t dataSize;
    void *pDescriptor;
};

struct vkGetDescriptorSetHostMappingVALVE_params
{
    VkDevice device;
    VkDescriptorSet DECLSPEC_ALIGN(8) descriptorSet;
    void **ppData;
};

struct vkGetDescriptorSetLayoutBindingOffsetEXT_params
{
    VkDevice device;
    VkDescriptorSetLayout DECLSPEC_ALIGN(8) layout;
    uint32_t binding;
    VkDeviceSize *pOffset;
};

struct vkGetDescriptorSetLayoutHostMappingInfoVALVE_params
{
    VkDevice device;
    const VkDescriptorSetBindingReferenceVALVE *pBindingReference;
    VkDescriptorSetLayoutHostMappingInfoVALVE *pHostMapping;
};

struct vkGetDescriptorSetLayoutSizeEXT_params
{
    VkDevice device;
    VkDescriptorSetLayout DECLSPEC_ALIGN(8) layout;
    VkDeviceSize *pLayoutSizeInBytes;
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

struct vkGetDeviceFaultInfoEXT_params
{
    VkDevice device;
    VkDeviceFaultCountsEXT *pFaultCounts;
    VkDeviceFaultInfoEXT *pFaultInfo;
    VkResult result;
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
    VkResult result;
};

struct vkGetDeviceGroupSurfacePresentModesKHR_params
{
    VkDevice device;
    VkSurfaceKHR DECLSPEC_ALIGN(8) surface;
    VkDeviceGroupPresentModeFlagsKHR *pModes;
    VkResult result;
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

struct vkGetDeviceImageSubresourceLayout_params
{
    VkDevice device;
    const VkDeviceImageSubresourceInfo *pInfo;
    VkSubresourceLayout2 *pLayout;
};

struct vkGetDeviceImageSubresourceLayoutKHR_params
{
    VkDevice device;
    const VkDeviceImageSubresourceInfo *pInfo;
    VkSubresourceLayout2 *pLayout;
};

struct vkGetDeviceMemoryCommitment_params
{
    VkDevice device;
    VkDeviceMemory DECLSPEC_ALIGN(8) memory;
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

struct vkGetDeviceMicromapCompatibilityEXT_params
{
    VkDevice device;
    const VkMicromapVersionInfoEXT *pVersionInfo;
    VkAccelerationStructureCompatibilityKHR *pCompatibility;
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
    VkRenderPass DECLSPEC_ALIGN(8) renderpass;
    VkExtent2D *pMaxWorkgroupSize;
    VkResult result;
};

struct vkGetDeviceTensorMemoryRequirementsARM_params
{
    VkDevice device;
    const VkDeviceTensorMemoryRequirementsARM *pInfo;
    VkMemoryRequirements2 *pMemoryRequirements;
};

struct vkGetDynamicRenderingTilePropertiesQCOM_params
{
    VkDevice device;
    const VkRenderingInfo *pRenderingInfo;
    VkTilePropertiesQCOM *pProperties;
    VkResult result;
};

struct vkGetEncodedVideoSessionParametersKHR_params
{
    VkDevice device;
    const VkVideoEncodeSessionParametersGetInfoKHR *pVideoSessionParametersInfo;
    VkVideoEncodeSessionParametersFeedbackInfoKHR *pFeedbackInfo;
    size_t *pDataSize;
    void *pData;
    VkResult result;
};

struct vkGetEventStatus_params
{
    VkDevice device;
    VkEvent DECLSPEC_ALIGN(8) event;
    VkResult result;
};

struct vkGetFenceStatus_params
{
    VkDevice device;
    VkFence DECLSPEC_ALIGN(8) fence;
    VkResult result;
};

struct vkGetFramebufferTilePropertiesQCOM_params
{
    VkDevice device;
    VkFramebuffer DECLSPEC_ALIGN(8) framebuffer;
    uint32_t *pPropertiesCount;
    VkTilePropertiesQCOM *pProperties;
    VkResult result;
};

struct vkGetGeneratedCommandsMemoryRequirementsEXT_params
{
    VkDevice device;
    const VkGeneratedCommandsMemoryRequirementsInfoEXT *pInfo;
    VkMemoryRequirements2 *pMemoryRequirements;
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
    VkImage DECLSPEC_ALIGN(8) image;
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

struct vkGetImageOpaqueCaptureDescriptorDataEXT_params
{
    VkDevice device;
    const VkImageCaptureDescriptorDataInfoEXT *pInfo;
    void *pData;
    VkResult result;
};

struct vkGetImageSparseMemoryRequirements_params
{
    VkDevice device;
    VkImage DECLSPEC_ALIGN(8) image;
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
    VkImage DECLSPEC_ALIGN(8) image;
    const VkImageSubresource *pSubresource;
    VkSubresourceLayout *pLayout;
};

struct vkGetImageSubresourceLayout2_params
{
    VkDevice device;
    VkImage DECLSPEC_ALIGN(8) image;
    const VkImageSubresource2 *pSubresource;
    VkSubresourceLayout2 *pLayout;
};

struct vkGetImageSubresourceLayout2EXT_params
{
    VkDevice device;
    VkImage DECLSPEC_ALIGN(8) image;
    const VkImageSubresource2 *pSubresource;
    VkSubresourceLayout2 *pLayout;
};

struct vkGetImageSubresourceLayout2KHR_params
{
    VkDevice device;
    VkImage DECLSPEC_ALIGN(8) image;
    const VkImageSubresource2 *pSubresource;
    VkSubresourceLayout2 *pLayout;
};

struct vkGetImageViewAddressNVX_params
{
    VkDevice device;
    VkImageView DECLSPEC_ALIGN(8) imageView;
    VkImageViewAddressPropertiesNVX *pProperties;
    VkResult result;
};

struct vkGetImageViewHandle64NVX_params
{
    VkDevice device;
    const VkImageViewHandleInfoNVX *pInfo;
    uint64_t result;
};

struct vkGetImageViewHandleNVX_params
{
    VkDevice device;
    const VkImageViewHandleInfoNVX *pInfo;
    uint32_t result;
};

struct vkGetImageViewOpaqueCaptureDescriptorDataEXT_params
{
    VkDevice device;
    const VkImageViewCaptureDescriptorDataInfoEXT *pInfo;
    void *pData;
    VkResult result;
};

struct vkGetLatencyTimingsNV_params
{
    VkDevice device;
    VkSwapchainKHR DECLSPEC_ALIGN(8) swapchain;
    VkGetLatencyMarkerInfoNV *pLatencyMarkerInfo;
};

struct vkGetMemoryHostPointerPropertiesEXT_params
{
    VkDevice device;
    VkExternalMemoryHandleTypeFlagBits handleType;
    const void *pHostPointer;
    VkMemoryHostPointerPropertiesEXT *pMemoryHostPointerProperties;
    VkResult result;
};

struct vkGetMemoryWin32HandleKHR_params
{
    VkDevice device;
    const VkMemoryGetWin32HandleInfoKHR *pGetWin32HandleInfo;
    HANDLE *pHandle;
    VkResult result;
};

struct vkGetMemoryWin32HandlePropertiesKHR_params
{
    VkDevice device;
    VkExternalMemoryHandleTypeFlagBits handleType;
    HANDLE handle;
    VkMemoryWin32HandlePropertiesKHR *pMemoryWin32HandleProperties;
    VkResult result;
};

struct vkGetMicromapBuildSizesEXT_params
{
    VkDevice device;
    VkAccelerationStructureBuildTypeKHR buildType;
    const VkMicromapBuildInfoEXT *pBuildInfo;
    VkMicromapBuildSizesInfoEXT *pSizeInfo;
};

struct vkGetPartitionedAccelerationStructuresBuildSizesNV_params
{
    VkDevice device;
    const VkPartitionedAccelerationStructureInstancesInputNV *pInfo;
    VkAccelerationStructureBuildSizesInfoKHR *pSizeInfo;
};

struct vkGetPerformanceParameterINTEL_params
{
    VkDevice device;
    VkPerformanceParameterTypeINTEL parameter;
    VkPerformanceValueINTEL *pValue;
    VkResult result;
};

struct vkGetPhysicalDeviceCalibrateableTimeDomainsEXT_params
{
    VkPhysicalDevice physicalDevice;
    uint32_t *pTimeDomainCount;
    VkTimeDomainKHR *pTimeDomains;
    VkResult result;
};

struct vkGetPhysicalDeviceCalibrateableTimeDomainsKHR_params
{
    VkPhysicalDevice physicalDevice;
    uint32_t *pTimeDomainCount;
    VkTimeDomainKHR *pTimeDomains;
    VkResult result;
};

struct vkGetPhysicalDeviceCooperativeMatrixFlexibleDimensionsPropertiesNV_params
{
    VkPhysicalDevice physicalDevice;
    uint32_t *pPropertyCount;
    VkCooperativeMatrixFlexibleDimensionsPropertiesNV *pProperties;
    VkResult result;
};

struct vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR_params
{
    VkPhysicalDevice physicalDevice;
    uint32_t *pPropertyCount;
    VkCooperativeMatrixPropertiesKHR *pProperties;
    VkResult result;
};

struct vkGetPhysicalDeviceCooperativeMatrixPropertiesNV_params
{
    VkPhysicalDevice physicalDevice;
    uint32_t *pPropertyCount;
    VkCooperativeMatrixPropertiesNV *pProperties;
    VkResult result;
};

struct vkGetPhysicalDeviceCooperativeVectorPropertiesNV_params
{
    VkPhysicalDevice physicalDevice;
    uint32_t *pPropertyCount;
    VkCooperativeVectorPropertiesNV *pProperties;
    VkResult result;
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

struct vkGetPhysicalDeviceExternalTensorPropertiesARM_params
{
    VkPhysicalDevice physicalDevice;
    const VkPhysicalDeviceExternalTensorInfoARM *pExternalTensorInfo;
    VkExternalTensorPropertiesARM *pExternalTensorProperties;
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
    VkResult result;
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
    VkResult result;
};

struct vkGetPhysicalDeviceImageFormatProperties2_params
{
    VkPhysicalDevice physicalDevice;
    const VkPhysicalDeviceImageFormatInfo2 *pImageFormatInfo;
    VkImageFormatProperties2 *pImageFormatProperties;
    VkResult result;
};

struct vkGetPhysicalDeviceImageFormatProperties2KHR_params
{
    VkPhysicalDevice physicalDevice;
    const VkPhysicalDeviceImageFormatInfo2 *pImageFormatInfo;
    VkImageFormatProperties2 *pImageFormatProperties;
    VkResult result;
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

struct vkGetPhysicalDeviceOpticalFlowImageFormatsNV_params
{
    VkPhysicalDevice physicalDevice;
    const VkOpticalFlowImageFormatInfoNV *pOpticalFlowImageFormatInfo;
    uint32_t *pFormatCount;
    VkOpticalFlowImageFormatPropertiesNV *pImageFormatProperties;
    VkResult result;
};

struct vkGetPhysicalDevicePresentRectanglesKHR_params
{
    VkPhysicalDevice physicalDevice;
    VkSurfaceKHR DECLSPEC_ALIGN(8) surface;
    uint32_t *pRectCount;
    VkRect2D *pRects;
    VkResult result;
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

struct vkGetPhysicalDeviceQueueFamilyDataGraphProcessingEnginePropertiesARM_params
{
    VkPhysicalDevice physicalDevice;
    const VkPhysicalDeviceQueueFamilyDataGraphProcessingEngineInfoARM *pQueueFamilyDataGraphProcessingEngineInfo;
    VkQueueFamilyDataGraphProcessingEnginePropertiesARM *pQueueFamilyDataGraphProcessingEngineProperties;
};

struct vkGetPhysicalDeviceQueueFamilyDataGraphPropertiesARM_params
{
    VkPhysicalDevice physicalDevice;
    uint32_t queueFamilyIndex;
    uint32_t *pQueueFamilyDataGraphPropertyCount;
    VkQueueFamilyDataGraphPropertiesARM *pQueueFamilyDataGraphProperties;
    VkResult result;
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
    VkResult result;
};

struct vkGetPhysicalDeviceSurfaceCapabilities2KHR_params
{
    VkPhysicalDevice physicalDevice;
    const VkPhysicalDeviceSurfaceInfo2KHR *pSurfaceInfo;
    VkSurfaceCapabilities2KHR *pSurfaceCapabilities;
    VkResult result;
};

struct vkGetPhysicalDeviceSurfaceCapabilitiesKHR_params
{
    VkPhysicalDevice physicalDevice;
    VkSurfaceKHR DECLSPEC_ALIGN(8) surface;
    VkSurfaceCapabilitiesKHR *pSurfaceCapabilities;
    VkResult result;
};

struct vkGetPhysicalDeviceSurfaceFormats2KHR_params
{
    VkPhysicalDevice physicalDevice;
    const VkPhysicalDeviceSurfaceInfo2KHR *pSurfaceInfo;
    uint32_t *pSurfaceFormatCount;
    VkSurfaceFormat2KHR *pSurfaceFormats;
    VkResult result;
};

struct vkGetPhysicalDeviceSurfaceFormatsKHR_params
{
    VkPhysicalDevice physicalDevice;
    VkSurfaceKHR DECLSPEC_ALIGN(8) surface;
    uint32_t *pSurfaceFormatCount;
    VkSurfaceFormatKHR *pSurfaceFormats;
    VkResult result;
};

struct vkGetPhysicalDeviceSurfacePresentModesKHR_params
{
    VkPhysicalDevice physicalDevice;
    VkSurfaceKHR DECLSPEC_ALIGN(8) surface;
    uint32_t *pPresentModeCount;
    VkPresentModeKHR *pPresentModes;
    VkResult result;
};

struct vkGetPhysicalDeviceSurfaceSupportKHR_params
{
    VkPhysicalDevice physicalDevice;
    uint32_t queueFamilyIndex;
    VkSurfaceKHR DECLSPEC_ALIGN(8) surface;
    VkBool32 *pSupported;
    VkResult result;
};

struct vkGetPhysicalDeviceToolProperties_params
{
    VkPhysicalDevice physicalDevice;
    uint32_t *pToolCount;
    VkPhysicalDeviceToolProperties *pToolProperties;
    VkResult result;
};

struct vkGetPhysicalDeviceToolPropertiesEXT_params
{
    VkPhysicalDevice physicalDevice;
    uint32_t *pToolCount;
    VkPhysicalDeviceToolProperties *pToolProperties;
    VkResult result;
};

struct vkGetPhysicalDeviceVideoCapabilitiesKHR_params
{
    VkPhysicalDevice physicalDevice;
    const VkVideoProfileInfoKHR *pVideoProfile;
    VkVideoCapabilitiesKHR *pCapabilities;
    VkResult result;
};

struct vkGetPhysicalDeviceVideoEncodeQualityLevelPropertiesKHR_params
{
    VkPhysicalDevice physicalDevice;
    const VkPhysicalDeviceVideoEncodeQualityLevelInfoKHR *pQualityLevelInfo;
    VkVideoEncodeQualityLevelPropertiesKHR *pQualityLevelProperties;
    VkResult result;
};

struct vkGetPhysicalDeviceVideoFormatPropertiesKHR_params
{
    VkPhysicalDevice physicalDevice;
    const VkPhysicalDeviceVideoFormatInfoKHR *pVideoFormatInfo;
    uint32_t *pVideoFormatPropertyCount;
    VkVideoFormatPropertiesKHR *pVideoFormatProperties;
    VkResult result;
};

struct vkGetPhysicalDeviceWin32PresentationSupportKHR_params
{
    VkPhysicalDevice physicalDevice;
    uint32_t queueFamilyIndex;
    VkBool32 result;
};

struct vkGetPipelineBinaryDataKHR_params
{
    VkDevice device;
    const VkPipelineBinaryDataInfoKHR *pInfo;
    VkPipelineBinaryKeyKHR *pPipelineBinaryKey;
    size_t *pPipelineBinaryDataSize;
    void *pPipelineBinaryData;
    VkResult result;
};

struct vkGetPipelineCacheData_params
{
    VkDevice device;
    VkPipelineCache DECLSPEC_ALIGN(8) pipelineCache;
    size_t *pDataSize;
    void *pData;
    VkResult result;
};

struct vkGetPipelineExecutableInternalRepresentationsKHR_params
{
    VkDevice device;
    const VkPipelineExecutableInfoKHR *pExecutableInfo;
    uint32_t *pInternalRepresentationCount;
    VkPipelineExecutableInternalRepresentationKHR *pInternalRepresentations;
    VkResult result;
};

struct vkGetPipelineExecutablePropertiesKHR_params
{
    VkDevice device;
    const VkPipelineInfoKHR *pPipelineInfo;
    uint32_t *pExecutableCount;
    VkPipelineExecutablePropertiesKHR *pProperties;
    VkResult result;
};

struct vkGetPipelineExecutableStatisticsKHR_params
{
    VkDevice device;
    const VkPipelineExecutableInfoKHR *pExecutableInfo;
    uint32_t *pStatisticCount;
    VkPipelineExecutableStatisticKHR *pStatistics;
    VkResult result;
};

struct vkGetPipelineIndirectDeviceAddressNV_params
{
    VkDevice device;
    const VkPipelineIndirectDeviceAddressInfoNV *pInfo;
    VkDeviceAddress result;
};

struct vkGetPipelineIndirectMemoryRequirementsNV_params
{
    VkDevice device;
    const VkComputePipelineCreateInfo *pCreateInfo;
    VkMemoryRequirements2 *pMemoryRequirements;
};

struct vkGetPipelineKeyKHR_params
{
    VkDevice device;
    const VkPipelineCreateInfoKHR *pPipelineCreateInfo;
    VkPipelineBinaryKeyKHR *pPipelineKey;
    VkResult result;
};

struct vkGetPipelinePropertiesEXT_params
{
    VkDevice device;
    const VkPipelineInfoEXT *pPipelineInfo;
    VkBaseOutStructure *pPipelineProperties;
    VkResult result;
};

struct vkGetPrivateData_params
{
    VkDevice device;
    VkObjectType objectType;
    uint64_t DECLSPEC_ALIGN(8) objectHandle;
    VkPrivateDataSlot DECLSPEC_ALIGN(8) privateDataSlot;
    uint64_t *pData;
};

struct vkGetPrivateDataEXT_params
{
    VkDevice device;
    VkObjectType objectType;
    uint64_t DECLSPEC_ALIGN(8) objectHandle;
    VkPrivateDataSlot DECLSPEC_ALIGN(8) privateDataSlot;
    uint64_t *pData;
};

struct vkGetQueryPoolResults_params
{
    VkDevice device;
    VkQueryPool DECLSPEC_ALIGN(8) queryPool;
    uint32_t firstQuery;
    uint32_t queryCount;
    size_t dataSize;
    void *pData;
    VkDeviceSize DECLSPEC_ALIGN(8) stride;
    VkQueryResultFlags flags;
    VkResult result;
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
    VkPipeline DECLSPEC_ALIGN(8) pipeline;
    uint32_t firstGroup;
    uint32_t groupCount;
    size_t dataSize;
    void *pData;
    VkResult result;
};

struct vkGetRayTracingShaderGroupHandlesKHR_params
{
    VkDevice device;
    VkPipeline DECLSPEC_ALIGN(8) pipeline;
    uint32_t firstGroup;
    uint32_t groupCount;
    size_t dataSize;
    void *pData;
    VkResult result;
};

struct vkGetRayTracingShaderGroupHandlesNV_params
{
    VkDevice device;
    VkPipeline DECLSPEC_ALIGN(8) pipeline;
    uint32_t firstGroup;
    uint32_t groupCount;
    size_t dataSize;
    void *pData;
    VkResult result;
};

struct vkGetRayTracingShaderGroupStackSizeKHR_params
{
    VkDevice device;
    VkPipeline DECLSPEC_ALIGN(8) pipeline;
    uint32_t group;
    VkShaderGroupShaderKHR groupShader;
    VkDeviceSize result;
};

struct vkGetRenderAreaGranularity_params
{
    VkDevice device;
    VkRenderPass DECLSPEC_ALIGN(8) renderPass;
    VkExtent2D *pGranularity;
};

struct vkGetRenderingAreaGranularity_params
{
    VkDevice device;
    const VkRenderingAreaInfo *pRenderingAreaInfo;
    VkExtent2D *pGranularity;
};

struct vkGetRenderingAreaGranularityKHR_params
{
    VkDevice device;
    const VkRenderingAreaInfo *pRenderingAreaInfo;
    VkExtent2D *pGranularity;
};

struct vkGetSamplerOpaqueCaptureDescriptorDataEXT_params
{
    VkDevice device;
    const VkSamplerCaptureDescriptorDataInfoEXT *pInfo;
    void *pData;
    VkResult result;
};

struct vkGetSemaphoreCounterValue_params
{
    VkDevice device;
    VkSemaphore DECLSPEC_ALIGN(8) semaphore;
    uint64_t *pValue;
    VkResult result;
};

struct vkGetSemaphoreCounterValueKHR_params
{
    VkDevice device;
    VkSemaphore DECLSPEC_ALIGN(8) semaphore;
    uint64_t *pValue;
    VkResult result;
};

struct vkGetSemaphoreWin32HandleKHR_params
{
    VkDevice device;
    const VkSemaphoreGetWin32HandleInfoKHR *pGetWin32HandleInfo;
    HANDLE *pHandle;
    VkResult result;
};

struct vkGetShaderBinaryDataEXT_params
{
    VkDevice device;
    VkShaderEXT DECLSPEC_ALIGN(8) shader;
    size_t *pDataSize;
    void *pData;
    VkResult result;
};

struct vkGetShaderInfoAMD_params
{
    VkDevice device;
    VkPipeline DECLSPEC_ALIGN(8) pipeline;
    VkShaderStageFlagBits shaderStage;
    VkShaderInfoTypeAMD infoType;
    size_t *pInfoSize;
    void *pInfo;
    VkResult result;
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
    VkShaderModule DECLSPEC_ALIGN(8) shaderModule;
    VkShaderModuleIdentifierEXT *pIdentifier;
};

struct vkGetSwapchainImagesKHR_params
{
    VkDevice device;
    VkSwapchainKHR DECLSPEC_ALIGN(8) swapchain;
    uint32_t *pSwapchainImageCount;
    VkImage *pSwapchainImages;
    VkResult result;
};

struct vkGetTensorMemoryRequirementsARM_params
{
    VkDevice device;
    const VkTensorMemoryRequirementsInfoARM *pInfo;
    VkMemoryRequirements2 *pMemoryRequirements;
};

struct vkGetTensorOpaqueCaptureDescriptorDataARM_params
{
    VkDevice device;
    const VkTensorCaptureDescriptorDataInfoARM *pInfo;
    void *pData;
    VkResult result;
};

struct vkGetTensorViewOpaqueCaptureDescriptorDataARM_params
{
    VkDevice device;
    const VkTensorViewCaptureDescriptorDataInfoARM *pInfo;
    void *pData;
    VkResult result;
};

struct vkGetValidationCacheDataEXT_params
{
    VkDevice device;
    VkValidationCacheEXT DECLSPEC_ALIGN(8) validationCache;
    size_t *pDataSize;
    void *pData;
    VkResult result;
};

struct vkGetVideoSessionMemoryRequirementsKHR_params
{
    VkDevice device;
    VkVideoSessionKHR DECLSPEC_ALIGN(8) videoSession;
    uint32_t *pMemoryRequirementsCount;
    VkVideoSessionMemoryRequirementsKHR *pMemoryRequirements;
    VkResult result;
};

struct vkImportSemaphoreWin32HandleKHR_params
{
    VkDevice device;
    const VkImportSemaphoreWin32HandleInfoKHR *pImportSemaphoreWin32HandleInfo;
    VkResult result;
};

struct vkInitializePerformanceApiINTEL_params
{
    VkDevice device;
    const VkInitializePerformanceApiInfoINTEL *pInitializeInfo;
    VkResult result;
};

struct vkInvalidateMappedMemoryRanges_params
{
    VkDevice device;
    uint32_t memoryRangeCount;
    const VkMappedMemoryRange *pMemoryRanges;
    VkResult result;
};

struct vkLatencySleepNV_params
{
    VkDevice device;
    VkSwapchainKHR DECLSPEC_ALIGN(8) swapchain;
    const VkLatencySleepInfoNV *pSleepInfo;
    VkResult result;
};

struct vkMapMemory_params
{
    VkDevice device;
    VkDeviceMemory DECLSPEC_ALIGN(8) memory;
    VkDeviceSize DECLSPEC_ALIGN(8) offset;
    VkDeviceSize DECLSPEC_ALIGN(8) size;
    VkMemoryMapFlags flags;
    void **ppData;
    VkResult result;
};

struct vkMapMemory2_params
{
    VkDevice device;
    const VkMemoryMapInfo *pMemoryMapInfo;
    void **ppData;
    VkResult result;
};

struct vkMapMemory2KHR_params
{
    VkDevice device;
    const VkMemoryMapInfo *pMemoryMapInfo;
    void **ppData;
    VkResult result;
};

struct vkMergePipelineCaches_params
{
    VkDevice device;
    VkPipelineCache DECLSPEC_ALIGN(8) dstCache;
    uint32_t srcCacheCount;
    const VkPipelineCache *pSrcCaches;
    VkResult result;
};

struct vkMergeValidationCachesEXT_params
{
    VkDevice device;
    VkValidationCacheEXT DECLSPEC_ALIGN(8) dstCache;
    uint32_t srcCacheCount;
    const VkValidationCacheEXT *pSrcCaches;
    VkResult result;
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
    VkFence DECLSPEC_ALIGN(8) fence;
    VkResult result;
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

struct vkQueueNotifyOutOfBandNV_params
{
    VkQueue queue;
    const VkOutOfBandQueueTypeInfoNV *pQueueTypeInfo;
};

struct vkQueuePresentKHR_params
{
    VkQueue queue;
    const VkPresentInfoKHR *pPresentInfo;
    VkResult result;
};

struct vkQueueSetPerformanceConfigurationINTEL_params
{
    VkQueue queue;
    VkPerformanceConfigurationINTEL DECLSPEC_ALIGN(8) configuration;
    VkResult result;
};

struct vkQueueSubmit_params
{
    VkQueue queue;
    uint32_t submitCount;
    const VkSubmitInfo *pSubmits;
    VkFence DECLSPEC_ALIGN(8) fence;
    VkResult result;
};

struct vkQueueSubmit2_params
{
    VkQueue queue;
    uint32_t submitCount;
    const VkSubmitInfo2 *pSubmits;
    VkFence DECLSPEC_ALIGN(8) fence;
    VkResult result;
};

struct vkQueueSubmit2KHR_params
{
    VkQueue queue;
    uint32_t submitCount;
    const VkSubmitInfo2 *pSubmits;
    VkFence DECLSPEC_ALIGN(8) fence;
    VkResult result;
};

struct vkQueueWaitIdle_params
{
    VkQueue queue;
    VkResult result;
};

struct vkReleaseCapturedPipelineDataKHR_params
{
    VkDevice device;
    const VkReleaseCapturedPipelineDataInfoKHR *pInfo;
    const VkAllocationCallbacks *pAllocator;
    VkResult result;
};

struct vkReleasePerformanceConfigurationINTEL_params
{
    VkDevice device;
    VkPerformanceConfigurationINTEL DECLSPEC_ALIGN(8) configuration;
    VkResult result;
};

struct vkReleaseProfilingLockKHR_params
{
    VkDevice device;
};

struct vkReleaseSwapchainImagesEXT_params
{
    VkDevice device;
    const VkReleaseSwapchainImagesInfoKHR *pReleaseInfo;
    VkResult result;
};

struct vkReleaseSwapchainImagesKHR_params
{
    VkDevice device;
    const VkReleaseSwapchainImagesInfoKHR *pReleaseInfo;
    VkResult result;
};

struct vkResetCommandBuffer_params
{
    VkCommandBuffer commandBuffer;
    VkCommandBufferResetFlags flags;
    VkResult result;
};

struct vkResetCommandPool_params
{
    VkDevice device;
    VkCommandPool DECLSPEC_ALIGN(8) commandPool;
    VkCommandPoolResetFlags flags;
    VkResult result;
};

struct vkResetDescriptorPool_params
{
    VkDevice device;
    VkDescriptorPool DECLSPEC_ALIGN(8) descriptorPool;
    VkDescriptorPoolResetFlags flags;
    VkResult result;
};

struct vkResetEvent_params
{
    VkDevice device;
    VkEvent DECLSPEC_ALIGN(8) event;
    VkResult result;
};

struct vkResetFences_params
{
    VkDevice device;
    uint32_t fenceCount;
    const VkFence *pFences;
    VkResult result;
};

struct vkResetQueryPool_params
{
    VkDevice device;
    VkQueryPool DECLSPEC_ALIGN(8) queryPool;
    uint32_t firstQuery;
    uint32_t queryCount;
};

struct vkResetQueryPoolEXT_params
{
    VkDevice device;
    VkQueryPool DECLSPEC_ALIGN(8) queryPool;
    uint32_t firstQuery;
    uint32_t queryCount;
};

struct vkSetDebugUtilsObjectNameEXT_params
{
    VkDevice device;
    const VkDebugUtilsObjectNameInfoEXT *pNameInfo;
    VkResult result;
};

struct vkSetDebugUtilsObjectTagEXT_params
{
    VkDevice device;
    const VkDebugUtilsObjectTagInfoEXT *pTagInfo;
    VkResult result;
};

struct vkSetDeviceMemoryPriorityEXT_params
{
    VkDevice device;
    VkDeviceMemory DECLSPEC_ALIGN(8) memory;
    float priority;
};

struct vkSetEvent_params
{
    VkDevice device;
    VkEvent DECLSPEC_ALIGN(8) event;
    VkResult result;
};

struct vkSetHdrMetadataEXT_params
{
    VkDevice device;
    uint32_t swapchainCount;
    const VkSwapchainKHR *pSwapchains;
    const VkHdrMetadataEXT *pMetadata;
};

struct vkSetLatencyMarkerNV_params
{
    VkDevice device;
    VkSwapchainKHR DECLSPEC_ALIGN(8) swapchain;
    const VkSetLatencyMarkerInfoNV *pLatencyMarkerInfo;
};

struct vkSetLatencySleepModeNV_params
{
    VkDevice device;
    VkSwapchainKHR DECLSPEC_ALIGN(8) swapchain;
    const VkLatencySleepModeInfoNV *pSleepModeInfo;
    VkResult result;
};

struct vkSetPrivateData_params
{
    VkDevice device;
    VkObjectType objectType;
    uint64_t DECLSPEC_ALIGN(8) objectHandle;
    VkPrivateDataSlot DECLSPEC_ALIGN(8) privateDataSlot;
    uint64_t DECLSPEC_ALIGN(8) data;
    VkResult result;
};

struct vkSetPrivateDataEXT_params
{
    VkDevice device;
    VkObjectType objectType;
    uint64_t DECLSPEC_ALIGN(8) objectHandle;
    VkPrivateDataSlot DECLSPEC_ALIGN(8) privateDataSlot;
    uint64_t DECLSPEC_ALIGN(8) data;
    VkResult result;
};

struct vkSignalSemaphore_params
{
    VkDevice device;
    const VkSemaphoreSignalInfo *pSignalInfo;
    VkResult result;
};

struct vkSignalSemaphoreKHR_params
{
    VkDevice device;
    const VkSemaphoreSignalInfo *pSignalInfo;
    VkResult result;
};

struct vkSubmitDebugUtilsMessageEXT_params
{
    VkInstance instance;
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity;
    VkDebugUtilsMessageTypeFlagsEXT messageTypes;
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData;
};

struct vkTransitionImageLayout_params
{
    VkDevice device;
    uint32_t transitionCount;
    const VkHostImageLayoutTransitionInfo *pTransitions;
    VkResult result;
};

struct vkTransitionImageLayoutEXT_params
{
    VkDevice device;
    uint32_t transitionCount;
    const VkHostImageLayoutTransitionInfo *pTransitions;
    VkResult result;
};

struct vkTrimCommandPool_params
{
    VkDevice device;
    VkCommandPool DECLSPEC_ALIGN(8) commandPool;
    VkCommandPoolTrimFlags flags;
};

struct vkTrimCommandPoolKHR_params
{
    VkDevice device;
    VkCommandPool DECLSPEC_ALIGN(8) commandPool;
    VkCommandPoolTrimFlags flags;
};

struct vkUninitializePerformanceApiINTEL_params
{
    VkDevice device;
};

struct vkUnmapMemory_params
{
    VkDevice device;
    VkDeviceMemory DECLSPEC_ALIGN(8) memory;
};

struct vkUnmapMemory2_params
{
    VkDevice device;
    const VkMemoryUnmapInfo *pMemoryUnmapInfo;
    VkResult result;
};

struct vkUnmapMemory2KHR_params
{
    VkDevice device;
    const VkMemoryUnmapInfo *pMemoryUnmapInfo;
    VkResult result;
};

struct vkUpdateDescriptorSetWithTemplate_params
{
    VkDevice device;
    VkDescriptorSet DECLSPEC_ALIGN(8) descriptorSet;
    VkDescriptorUpdateTemplate DECLSPEC_ALIGN(8) descriptorUpdateTemplate;
    const void *pData;
};

struct vkUpdateDescriptorSetWithTemplateKHR_params
{
    VkDevice device;
    VkDescriptorSet DECLSPEC_ALIGN(8) descriptorSet;
    VkDescriptorUpdateTemplate DECLSPEC_ALIGN(8) descriptorUpdateTemplate;
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

struct vkUpdateIndirectExecutionSetPipelineEXT_params
{
    VkDevice device;
    VkIndirectExecutionSetEXT DECLSPEC_ALIGN(8) indirectExecutionSet;
    uint32_t executionSetWriteCount;
    const VkWriteIndirectExecutionSetPipelineEXT *pExecutionSetWrites;
};

struct vkUpdateIndirectExecutionSetShaderEXT_params
{
    VkDevice device;
    VkIndirectExecutionSetEXT DECLSPEC_ALIGN(8) indirectExecutionSet;
    uint32_t executionSetWriteCount;
    const VkWriteIndirectExecutionSetShaderEXT *pExecutionSetWrites;
};

struct vkUpdateVideoSessionParametersKHR_params
{
    VkDevice device;
    VkVideoSessionParametersKHR DECLSPEC_ALIGN(8) videoSessionParameters;
    const VkVideoSessionParametersUpdateInfoKHR *pUpdateInfo;
    VkResult result;
};

struct vkWaitForFences_params
{
    VkDevice device;
    uint32_t fenceCount;
    const VkFence *pFences;
    VkBool32 waitAll;
    uint64_t DECLSPEC_ALIGN(8) timeout;
    VkResult result;
};

struct vkWaitForPresent2KHR_params
{
    VkDevice device;
    VkSwapchainKHR DECLSPEC_ALIGN(8) swapchain;
    const VkPresentWait2InfoKHR *pPresentWait2Info;
    VkResult result;
};

struct vkWaitForPresentKHR_params
{
    VkDevice device;
    VkSwapchainKHR DECLSPEC_ALIGN(8) swapchain;
    uint64_t DECLSPEC_ALIGN(8) presentId;
    uint64_t DECLSPEC_ALIGN(8) timeout;
    VkResult result;
};

struct vkWaitSemaphores_params
{
    VkDevice device;
    const VkSemaphoreWaitInfo *pWaitInfo;
    uint64_t DECLSPEC_ALIGN(8) timeout;
    VkResult result;
};

struct vkWaitSemaphoresKHR_params
{
    VkDevice device;
    const VkSemaphoreWaitInfo *pWaitInfo;
    uint64_t DECLSPEC_ALIGN(8) timeout;
    VkResult result;
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
    VkResult result;
};

struct vkWriteMicromapsPropertiesEXT_params
{
    VkDevice device;
    uint32_t micromapCount;
    const VkMicromapEXT *pMicromaps;
    VkQueryType queryType;
    size_t dataSize;
    void *pData;
    size_t stride;
    VkResult result;
};

#endif /* __WINE_VULKAN_LOADER_THUNKS_H */
