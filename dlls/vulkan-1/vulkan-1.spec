# Automatically generated from Vulkan vk.xml; DO NOT EDIT!
#
# This file is generated from Vulkan vk.xml file covered
# by the following copyright and permission notice:
#
# Copyright 2015-2024 The Khronos Group Inc.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

@ stdcall vkAcquireNextImage2KHR(ptr ptr ptr) winevulkan.vkAcquireNextImage2KHR
@ stdcall vkAcquireNextImageKHR(ptr int64 int64 int64 int64 ptr) winevulkan.vkAcquireNextImageKHR
@ stdcall vkAllocateCommandBuffers(ptr ptr ptr) winevulkan.vkAllocateCommandBuffers
@ stdcall vkAllocateDescriptorSets(ptr ptr ptr) winevulkan.vkAllocateDescriptorSets
@ stdcall vkAllocateMemory(ptr ptr ptr ptr) winevulkan.vkAllocateMemory
@ stdcall vkBeginCommandBuffer(ptr ptr) winevulkan.vkBeginCommandBuffer
@ stdcall vkBindBufferMemory(ptr int64 int64 int64) winevulkan.vkBindBufferMemory
@ stdcall vkBindBufferMemory2(ptr long ptr) winevulkan.vkBindBufferMemory2
@ stdcall vkBindImageMemory(ptr int64 int64 int64) winevulkan.vkBindImageMemory
@ stdcall vkBindImageMemory2(ptr long ptr) winevulkan.vkBindImageMemory2
@ stdcall vkCmdBeginQuery(ptr int64 long long) winevulkan.vkCmdBeginQuery
@ stdcall vkCmdBeginRenderPass(ptr ptr long) winevulkan.vkCmdBeginRenderPass
@ stdcall vkCmdBeginRenderPass2(ptr ptr ptr) winevulkan.vkCmdBeginRenderPass2
@ stdcall vkCmdBeginRendering(ptr ptr) winevulkan.vkCmdBeginRendering
@ stdcall vkCmdBindDescriptorSets(ptr long int64 long long ptr long ptr) winevulkan.vkCmdBindDescriptorSets
@ stdcall vkCmdBindIndexBuffer(ptr int64 int64 long) winevulkan.vkCmdBindIndexBuffer
@ stdcall vkCmdBindPipeline(ptr long int64) winevulkan.vkCmdBindPipeline
@ stdcall vkCmdBindVertexBuffers(ptr long long ptr ptr) winevulkan.vkCmdBindVertexBuffers
@ stdcall vkCmdBindVertexBuffers2(ptr long long ptr ptr ptr ptr) winevulkan.vkCmdBindVertexBuffers2
@ stdcall vkCmdBlitImage(ptr int64 long int64 long long ptr long) winevulkan.vkCmdBlitImage
@ stdcall vkCmdBlitImage2(ptr ptr) winevulkan.vkCmdBlitImage2
@ stdcall vkCmdClearAttachments(ptr long ptr long ptr) winevulkan.vkCmdClearAttachments
@ stdcall vkCmdClearColorImage(ptr int64 long ptr long ptr) winevulkan.vkCmdClearColorImage
@ stdcall vkCmdClearDepthStencilImage(ptr int64 long ptr long ptr) winevulkan.vkCmdClearDepthStencilImage
@ stdcall vkCmdCopyBuffer(ptr int64 int64 long ptr) winevulkan.vkCmdCopyBuffer
@ stdcall vkCmdCopyBuffer2(ptr ptr) winevulkan.vkCmdCopyBuffer2
@ stdcall vkCmdCopyBufferToImage(ptr int64 int64 long long ptr) winevulkan.vkCmdCopyBufferToImage
@ stdcall vkCmdCopyBufferToImage2(ptr ptr) winevulkan.vkCmdCopyBufferToImage2
@ stdcall vkCmdCopyImage(ptr int64 long int64 long long ptr) winevulkan.vkCmdCopyImage
@ stdcall vkCmdCopyImage2(ptr ptr) winevulkan.vkCmdCopyImage2
@ stdcall vkCmdCopyImageToBuffer(ptr int64 long int64 long ptr) winevulkan.vkCmdCopyImageToBuffer
@ stdcall vkCmdCopyImageToBuffer2(ptr ptr) winevulkan.vkCmdCopyImageToBuffer2
@ stdcall vkCmdCopyQueryPoolResults(ptr int64 long long int64 int64 int64 long) winevulkan.vkCmdCopyQueryPoolResults
@ stdcall vkCmdDispatch(ptr long long long) winevulkan.vkCmdDispatch
@ stdcall vkCmdDispatchBase(ptr long long long long long long) winevulkan.vkCmdDispatchBase
@ stdcall vkCmdDispatchIndirect(ptr int64 int64) winevulkan.vkCmdDispatchIndirect
@ stdcall vkCmdDraw(ptr long long long long) winevulkan.vkCmdDraw
@ stdcall vkCmdDrawIndexed(ptr long long long long long) winevulkan.vkCmdDrawIndexed
@ stdcall vkCmdDrawIndexedIndirect(ptr int64 int64 long long) winevulkan.vkCmdDrawIndexedIndirect
@ stdcall vkCmdDrawIndexedIndirectCount(ptr int64 int64 int64 int64 long long) winevulkan.vkCmdDrawIndexedIndirectCount
@ stdcall vkCmdDrawIndirect(ptr int64 int64 long long) winevulkan.vkCmdDrawIndirect
@ stdcall vkCmdDrawIndirectCount(ptr int64 int64 int64 int64 long long) winevulkan.vkCmdDrawIndirectCount
@ stdcall vkCmdEndQuery(ptr int64 long) winevulkan.vkCmdEndQuery
@ stdcall vkCmdEndRenderPass(ptr) winevulkan.vkCmdEndRenderPass
@ stdcall vkCmdEndRenderPass2(ptr ptr) winevulkan.vkCmdEndRenderPass2
@ stdcall vkCmdEndRendering(ptr) winevulkan.vkCmdEndRendering
@ stdcall vkCmdExecuteCommands(ptr long ptr) winevulkan.vkCmdExecuteCommands
@ stdcall vkCmdFillBuffer(ptr int64 int64 int64 long) winevulkan.vkCmdFillBuffer
@ stdcall vkCmdNextSubpass(ptr long) winevulkan.vkCmdNextSubpass
@ stdcall vkCmdNextSubpass2(ptr ptr ptr) winevulkan.vkCmdNextSubpass2
@ stdcall vkCmdPipelineBarrier(ptr long long long long ptr long ptr long ptr) winevulkan.vkCmdPipelineBarrier
@ stdcall vkCmdPipelineBarrier2(ptr ptr) winevulkan.vkCmdPipelineBarrier2
@ stdcall vkCmdPushConstants(ptr int64 long long long ptr) winevulkan.vkCmdPushConstants
@ stdcall vkCmdResetEvent(ptr int64 long) winevulkan.vkCmdResetEvent
@ stdcall vkCmdResetEvent2(ptr int64 int64) winevulkan.vkCmdResetEvent2
@ stdcall vkCmdResetQueryPool(ptr int64 long long) winevulkan.vkCmdResetQueryPool
@ stdcall vkCmdResolveImage(ptr int64 long int64 long long ptr) winevulkan.vkCmdResolveImage
@ stdcall vkCmdResolveImage2(ptr ptr) winevulkan.vkCmdResolveImage2
@ stdcall vkCmdSetBlendConstants(ptr ptr) winevulkan.vkCmdSetBlendConstants
@ stdcall vkCmdSetCullMode(ptr long) winevulkan.vkCmdSetCullMode
@ stdcall vkCmdSetDepthBias(ptr float float float) winevulkan.vkCmdSetDepthBias
@ stdcall vkCmdSetDepthBiasEnable(ptr long) winevulkan.vkCmdSetDepthBiasEnable
@ stdcall vkCmdSetDepthBounds(ptr float float) winevulkan.vkCmdSetDepthBounds
@ stdcall vkCmdSetDepthBoundsTestEnable(ptr long) winevulkan.vkCmdSetDepthBoundsTestEnable
@ stdcall vkCmdSetDepthCompareOp(ptr long) winevulkan.vkCmdSetDepthCompareOp
@ stdcall vkCmdSetDepthTestEnable(ptr long) winevulkan.vkCmdSetDepthTestEnable
@ stdcall vkCmdSetDepthWriteEnable(ptr long) winevulkan.vkCmdSetDepthWriteEnable
@ stdcall vkCmdSetDeviceMask(ptr long) winevulkan.vkCmdSetDeviceMask
@ stdcall vkCmdSetEvent(ptr int64 long) winevulkan.vkCmdSetEvent
@ stdcall vkCmdSetEvent2(ptr int64 ptr) winevulkan.vkCmdSetEvent2
@ stdcall vkCmdSetFrontFace(ptr long) winevulkan.vkCmdSetFrontFace
@ stdcall vkCmdSetLineWidth(ptr float) winevulkan.vkCmdSetLineWidth
@ stdcall vkCmdSetPrimitiveRestartEnable(ptr long) winevulkan.vkCmdSetPrimitiveRestartEnable
@ stdcall vkCmdSetPrimitiveTopology(ptr long) winevulkan.vkCmdSetPrimitiveTopology
@ stdcall vkCmdSetRasterizerDiscardEnable(ptr long) winevulkan.vkCmdSetRasterizerDiscardEnable
@ stdcall vkCmdSetScissor(ptr long long ptr) winevulkan.vkCmdSetScissor
@ stdcall vkCmdSetScissorWithCount(ptr long ptr) winevulkan.vkCmdSetScissorWithCount
@ stdcall vkCmdSetStencilCompareMask(ptr long long) winevulkan.vkCmdSetStencilCompareMask
@ stdcall vkCmdSetStencilOp(ptr long long long long long) winevulkan.vkCmdSetStencilOp
@ stdcall vkCmdSetStencilReference(ptr long long) winevulkan.vkCmdSetStencilReference
@ stdcall vkCmdSetStencilTestEnable(ptr long) winevulkan.vkCmdSetStencilTestEnable
@ stdcall vkCmdSetStencilWriteMask(ptr long long) winevulkan.vkCmdSetStencilWriteMask
@ stdcall vkCmdSetViewport(ptr long long ptr) winevulkan.vkCmdSetViewport
@ stdcall vkCmdSetViewportWithCount(ptr long ptr) winevulkan.vkCmdSetViewportWithCount
@ stdcall vkCmdUpdateBuffer(ptr int64 int64 int64 ptr) winevulkan.vkCmdUpdateBuffer
@ stdcall vkCmdWaitEvents(ptr long ptr long long long ptr long ptr long ptr) winevulkan.vkCmdWaitEvents
@ stdcall vkCmdWaitEvents2(ptr long ptr ptr) winevulkan.vkCmdWaitEvents2
@ stdcall vkCmdWriteTimestamp(ptr long int64 long) winevulkan.vkCmdWriteTimestamp
@ stdcall vkCmdWriteTimestamp2(ptr int64 int64 long) winevulkan.vkCmdWriteTimestamp2
@ stdcall vkCreateBuffer(ptr ptr ptr ptr) winevulkan.vkCreateBuffer
@ stdcall vkCreateBufferView(ptr ptr ptr ptr) winevulkan.vkCreateBufferView
@ stdcall vkCreateCommandPool(ptr ptr ptr ptr) winevulkan.vkCreateCommandPool
@ stdcall vkCreateComputePipelines(ptr int64 long ptr ptr ptr) winevulkan.vkCreateComputePipelines
@ stdcall vkCreateDescriptorPool(ptr ptr ptr ptr) winevulkan.vkCreateDescriptorPool
@ stdcall vkCreateDescriptorSetLayout(ptr ptr ptr ptr) winevulkan.vkCreateDescriptorSetLayout
@ stdcall vkCreateDescriptorUpdateTemplate(ptr ptr ptr ptr) winevulkan.vkCreateDescriptorUpdateTemplate
@ stdcall vkCreateDevice(ptr ptr ptr ptr) winevulkan.vkCreateDevice
@ stub vkCreateDisplayModeKHR
@ stub vkCreateDisplayPlaneSurfaceKHR
@ stdcall vkCreateEvent(ptr ptr ptr ptr) winevulkan.vkCreateEvent
@ stdcall vkCreateFence(ptr ptr ptr ptr) winevulkan.vkCreateFence
@ stdcall vkCreateFramebuffer(ptr ptr ptr ptr) winevulkan.vkCreateFramebuffer
@ stdcall vkCreateGraphicsPipelines(ptr int64 long ptr ptr ptr) winevulkan.vkCreateGraphicsPipelines
@ stdcall vkCreateImage(ptr ptr ptr ptr) winevulkan.vkCreateImage
@ stdcall vkCreateImageView(ptr ptr ptr ptr) winevulkan.vkCreateImageView
@ stdcall vkCreateInstance(ptr ptr ptr) winevulkan.vkCreateInstance
@ stdcall vkCreatePipelineCache(ptr ptr ptr ptr) winevulkan.vkCreatePipelineCache
@ stdcall vkCreatePipelineLayout(ptr ptr ptr ptr) winevulkan.vkCreatePipelineLayout
@ stdcall vkCreatePrivateDataSlot(ptr ptr ptr ptr) winevulkan.vkCreatePrivateDataSlot
@ stdcall vkCreateQueryPool(ptr ptr ptr ptr) winevulkan.vkCreateQueryPool
@ stdcall vkCreateRenderPass(ptr ptr ptr ptr) winevulkan.vkCreateRenderPass
@ stdcall vkCreateRenderPass2(ptr ptr ptr ptr) winevulkan.vkCreateRenderPass2
@ stdcall vkCreateSampler(ptr ptr ptr ptr) winevulkan.vkCreateSampler
@ stdcall vkCreateSamplerYcbcrConversion(ptr ptr ptr ptr) winevulkan.vkCreateSamplerYcbcrConversion
@ stdcall vkCreateSemaphore(ptr ptr ptr ptr) winevulkan.vkCreateSemaphore
@ stdcall vkCreateShaderModule(ptr ptr ptr ptr) winevulkan.vkCreateShaderModule
@ stub vkCreateSharedSwapchainsKHR
@ stdcall vkCreateSwapchainKHR(ptr ptr ptr ptr) winevulkan.vkCreateSwapchainKHR
@ stdcall vkCreateWin32SurfaceKHR(ptr ptr ptr ptr) winevulkan.vkCreateWin32SurfaceKHR
@ stdcall vkDestroyBuffer(ptr int64 ptr) winevulkan.vkDestroyBuffer
@ stdcall vkDestroyBufferView(ptr int64 ptr) winevulkan.vkDestroyBufferView
@ stdcall vkDestroyCommandPool(ptr int64 ptr) winevulkan.vkDestroyCommandPool
@ stdcall vkDestroyDescriptorPool(ptr int64 ptr) winevulkan.vkDestroyDescriptorPool
@ stdcall vkDestroyDescriptorSetLayout(ptr int64 ptr) winevulkan.vkDestroyDescriptorSetLayout
@ stdcall vkDestroyDescriptorUpdateTemplate(ptr int64 ptr) winevulkan.vkDestroyDescriptorUpdateTemplate
@ stdcall vkDestroyDevice(ptr ptr) winevulkan.vkDestroyDevice
@ stdcall vkDestroyEvent(ptr int64 ptr) winevulkan.vkDestroyEvent
@ stdcall vkDestroyFence(ptr int64 ptr) winevulkan.vkDestroyFence
@ stdcall vkDestroyFramebuffer(ptr int64 ptr) winevulkan.vkDestroyFramebuffer
@ stdcall vkDestroyImage(ptr int64 ptr) winevulkan.vkDestroyImage
@ stdcall vkDestroyImageView(ptr int64 ptr) winevulkan.vkDestroyImageView
@ stdcall vkDestroyInstance(ptr ptr) winevulkan.vkDestroyInstance
@ stdcall vkDestroyPipeline(ptr int64 ptr) winevulkan.vkDestroyPipeline
@ stdcall vkDestroyPipelineCache(ptr int64 ptr) winevulkan.vkDestroyPipelineCache
@ stdcall vkDestroyPipelineLayout(ptr int64 ptr) winevulkan.vkDestroyPipelineLayout
@ stdcall vkDestroyPrivateDataSlot(ptr int64 ptr) winevulkan.vkDestroyPrivateDataSlot
@ stdcall vkDestroyQueryPool(ptr int64 ptr) winevulkan.vkDestroyQueryPool
@ stdcall vkDestroyRenderPass(ptr int64 ptr) winevulkan.vkDestroyRenderPass
@ stdcall vkDestroySampler(ptr int64 ptr) winevulkan.vkDestroySampler
@ stdcall vkDestroySamplerYcbcrConversion(ptr int64 ptr) winevulkan.vkDestroySamplerYcbcrConversion
@ stdcall vkDestroySemaphore(ptr int64 ptr) winevulkan.vkDestroySemaphore
@ stdcall vkDestroyShaderModule(ptr int64 ptr) winevulkan.vkDestroyShaderModule
@ stdcall vkDestroySurfaceKHR(ptr int64 ptr) winevulkan.vkDestroySurfaceKHR
@ stdcall vkDestroySwapchainKHR(ptr int64 ptr) winevulkan.vkDestroySwapchainKHR
@ stdcall vkDeviceWaitIdle(ptr) winevulkan.vkDeviceWaitIdle
@ stdcall vkEndCommandBuffer(ptr) winevulkan.vkEndCommandBuffer
@ stdcall vkEnumerateDeviceExtensionProperties(ptr str ptr ptr) winevulkan.vkEnumerateDeviceExtensionProperties
@ stdcall vkEnumerateDeviceLayerProperties(ptr ptr ptr) winevulkan.vkEnumerateDeviceLayerProperties
@ stdcall vkEnumerateInstanceExtensionProperties(str ptr ptr) winevulkan.vkEnumerateInstanceExtensionProperties
@ stdcall vkEnumerateInstanceLayerProperties(ptr ptr) winevulkan.vkEnumerateInstanceLayerProperties
@ stdcall vkEnumerateInstanceVersion(ptr) winevulkan.vkEnumerateInstanceVersion
@ stdcall vkEnumeratePhysicalDeviceGroups(ptr ptr ptr) winevulkan.vkEnumeratePhysicalDeviceGroups
@ stdcall vkEnumeratePhysicalDevices(ptr ptr ptr) winevulkan.vkEnumeratePhysicalDevices
@ stdcall vkFlushMappedMemoryRanges(ptr long ptr) winevulkan.vkFlushMappedMemoryRanges
@ stdcall vkFreeCommandBuffers(ptr int64 long ptr) winevulkan.vkFreeCommandBuffers
@ stdcall vkFreeDescriptorSets(ptr int64 long ptr) winevulkan.vkFreeDescriptorSets
@ stdcall vkFreeMemory(ptr int64 ptr) winevulkan.vkFreeMemory
@ stdcall vkGetBufferDeviceAddress(ptr ptr) winevulkan.vkGetBufferDeviceAddress
@ stdcall vkGetBufferMemoryRequirements(ptr int64 ptr) winevulkan.vkGetBufferMemoryRequirements
@ stdcall vkGetBufferMemoryRequirements2(ptr ptr ptr) winevulkan.vkGetBufferMemoryRequirements2
@ stdcall vkGetBufferOpaqueCaptureAddress(ptr ptr) winevulkan.vkGetBufferOpaqueCaptureAddress
@ stub vkGetCommandPoolMemoryConsumption
@ stdcall vkGetDescriptorSetLayoutSupport(ptr ptr ptr) winevulkan.vkGetDescriptorSetLayoutSupport
@ stdcall vkGetDeviceBufferMemoryRequirements(ptr ptr ptr) winevulkan.vkGetDeviceBufferMemoryRequirements
@ stdcall vkGetDeviceGroupPeerMemoryFeatures(ptr long long long ptr) winevulkan.vkGetDeviceGroupPeerMemoryFeatures
@ stdcall vkGetDeviceGroupPresentCapabilitiesKHR(ptr ptr) winevulkan.vkGetDeviceGroupPresentCapabilitiesKHR
@ stdcall vkGetDeviceGroupSurfacePresentModesKHR(ptr int64 ptr) winevulkan.vkGetDeviceGroupSurfacePresentModesKHR
@ stdcall vkGetDeviceImageMemoryRequirements(ptr ptr ptr) winevulkan.vkGetDeviceImageMemoryRequirements
@ stdcall vkGetDeviceImageSparseMemoryRequirements(ptr ptr ptr ptr) winevulkan.vkGetDeviceImageSparseMemoryRequirements
@ stdcall vkGetDeviceMemoryCommitment(ptr int64 ptr) winevulkan.vkGetDeviceMemoryCommitment
@ stdcall vkGetDeviceMemoryOpaqueCaptureAddress(ptr ptr) winevulkan.vkGetDeviceMemoryOpaqueCaptureAddress
@ stdcall vkGetDeviceProcAddr(ptr str) winevulkan.vkGetDeviceProcAddr
@ stdcall vkGetDeviceQueue(ptr long long ptr) winevulkan.vkGetDeviceQueue
@ stdcall vkGetDeviceQueue2(ptr ptr ptr) winevulkan.vkGetDeviceQueue2
@ stub vkGetDisplayModePropertiesKHR
@ stub vkGetDisplayPlaneCapabilitiesKHR
@ stub vkGetDisplayPlaneSupportedDisplaysKHR
@ stdcall vkGetEventStatus(ptr int64) winevulkan.vkGetEventStatus
@ stub vkGetFaultData
@ stdcall vkGetFenceStatus(ptr int64) winevulkan.vkGetFenceStatus
@ stdcall vkGetImageMemoryRequirements(ptr int64 ptr) winevulkan.vkGetImageMemoryRequirements
@ stdcall vkGetImageMemoryRequirements2(ptr ptr ptr) winevulkan.vkGetImageMemoryRequirements2
@ stdcall vkGetImageSparseMemoryRequirements(ptr int64 ptr ptr) winevulkan.vkGetImageSparseMemoryRequirements
@ stdcall vkGetImageSparseMemoryRequirements2(ptr ptr ptr ptr) winevulkan.vkGetImageSparseMemoryRequirements2
@ stdcall vkGetImageSubresourceLayout(ptr int64 ptr ptr) winevulkan.vkGetImageSubresourceLayout
@ stdcall vkGetInstanceProcAddr(ptr str) winevulkan.vkGetInstanceProcAddr
@ stub vkGetPhysicalDeviceDisplayPlanePropertiesKHR
@ stub vkGetPhysicalDeviceDisplayPropertiesKHR
@ stdcall vkGetPhysicalDeviceExternalBufferProperties(ptr ptr ptr) winevulkan.vkGetPhysicalDeviceExternalBufferProperties
@ stdcall vkGetPhysicalDeviceExternalFenceProperties(ptr ptr ptr) winevulkan.vkGetPhysicalDeviceExternalFenceProperties
@ stdcall vkGetPhysicalDeviceExternalSemaphoreProperties(ptr ptr ptr) winevulkan.vkGetPhysicalDeviceExternalSemaphoreProperties
@ stdcall vkGetPhysicalDeviceFeatures(ptr ptr) winevulkan.vkGetPhysicalDeviceFeatures
@ stdcall vkGetPhysicalDeviceFeatures2(ptr ptr) winevulkan.vkGetPhysicalDeviceFeatures2
@ stdcall vkGetPhysicalDeviceFormatProperties(ptr long ptr) winevulkan.vkGetPhysicalDeviceFormatProperties
@ stdcall vkGetPhysicalDeviceFormatProperties2(ptr long ptr) winevulkan.vkGetPhysicalDeviceFormatProperties2
@ stdcall vkGetPhysicalDeviceImageFormatProperties(ptr long long long long long ptr) winevulkan.vkGetPhysicalDeviceImageFormatProperties
@ stdcall vkGetPhysicalDeviceImageFormatProperties2(ptr ptr ptr) winevulkan.vkGetPhysicalDeviceImageFormatProperties2
@ stdcall vkGetPhysicalDeviceMemoryProperties(ptr ptr) winevulkan.vkGetPhysicalDeviceMemoryProperties
@ stdcall vkGetPhysicalDeviceMemoryProperties2(ptr ptr) winevulkan.vkGetPhysicalDeviceMemoryProperties2
@ stdcall vkGetPhysicalDevicePresentRectanglesKHR(ptr int64 ptr ptr) winevulkan.vkGetPhysicalDevicePresentRectanglesKHR
@ stdcall vkGetPhysicalDeviceProperties(ptr ptr) winevulkan.vkGetPhysicalDeviceProperties
@ stdcall vkGetPhysicalDeviceProperties2(ptr ptr) winevulkan.vkGetPhysicalDeviceProperties2
@ stdcall vkGetPhysicalDeviceQueueFamilyProperties(ptr ptr ptr) winevulkan.vkGetPhysicalDeviceQueueFamilyProperties
@ stdcall vkGetPhysicalDeviceQueueFamilyProperties2(ptr ptr ptr) winevulkan.vkGetPhysicalDeviceQueueFamilyProperties2
@ stdcall vkGetPhysicalDeviceSparseImageFormatProperties(ptr long long long long long ptr ptr) winevulkan.vkGetPhysicalDeviceSparseImageFormatProperties
@ stdcall vkGetPhysicalDeviceSparseImageFormatProperties2(ptr ptr ptr ptr) winevulkan.vkGetPhysicalDeviceSparseImageFormatProperties2
@ stdcall vkGetPhysicalDeviceSurfaceCapabilities2KHR(ptr ptr ptr) winevulkan.vkGetPhysicalDeviceSurfaceCapabilities2KHR
@ stdcall vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ptr int64 ptr) winevulkan.vkGetPhysicalDeviceSurfaceCapabilitiesKHR
@ stdcall vkGetPhysicalDeviceSurfaceFormats2KHR(ptr ptr ptr ptr) winevulkan.vkGetPhysicalDeviceSurfaceFormats2KHR
@ stdcall vkGetPhysicalDeviceSurfaceFormatsKHR(ptr int64 ptr ptr) winevulkan.vkGetPhysicalDeviceSurfaceFormatsKHR
@ stdcall vkGetPhysicalDeviceSurfacePresentModesKHR(ptr int64 ptr ptr) winevulkan.vkGetPhysicalDeviceSurfacePresentModesKHR
@ stdcall vkGetPhysicalDeviceSurfaceSupportKHR(ptr long int64 ptr) winevulkan.vkGetPhysicalDeviceSurfaceSupportKHR
@ stdcall vkGetPhysicalDeviceToolProperties(ptr ptr ptr) winevulkan.vkGetPhysicalDeviceToolProperties
@ stdcall vkGetPhysicalDeviceWin32PresentationSupportKHR(ptr long) winevulkan.vkGetPhysicalDeviceWin32PresentationSupportKHR
@ stdcall vkGetPipelineCacheData(ptr int64 ptr ptr) winevulkan.vkGetPipelineCacheData
@ stdcall vkGetPrivateData(ptr long int64 int64 ptr) winevulkan.vkGetPrivateData
@ stdcall vkGetQueryPoolResults(ptr int64 long long long ptr int64 long) winevulkan.vkGetQueryPoolResults
@ stdcall vkGetRenderAreaGranularity(ptr int64 ptr) winevulkan.vkGetRenderAreaGranularity
@ stdcall vkGetSemaphoreCounterValue(ptr int64 ptr) winevulkan.vkGetSemaphoreCounterValue
@ stdcall vkGetSwapchainImagesKHR(ptr int64 ptr ptr) winevulkan.vkGetSwapchainImagesKHR
@ stdcall vkInvalidateMappedMemoryRanges(ptr long ptr) winevulkan.vkInvalidateMappedMemoryRanges
@ stdcall vkMapMemory(ptr int64 int64 int64 long ptr) winevulkan.vkMapMemory
@ stdcall vkMergePipelineCaches(ptr int64 long ptr) winevulkan.vkMergePipelineCaches
@ stdcall vkQueueBindSparse(ptr long ptr int64) winevulkan.vkQueueBindSparse
@ stdcall vkQueuePresentKHR(ptr ptr) winevulkan.vkQueuePresentKHR
@ stdcall vkQueueSubmit(ptr long ptr int64) winevulkan.vkQueueSubmit
@ stdcall vkQueueSubmit2(ptr long ptr int64) winevulkan.vkQueueSubmit2
@ stdcall vkQueueWaitIdle(ptr) winevulkan.vkQueueWaitIdle
@ stdcall vkResetCommandBuffer(ptr long) winevulkan.vkResetCommandBuffer
@ stdcall vkResetCommandPool(ptr int64 long) winevulkan.vkResetCommandPool
@ stdcall vkResetDescriptorPool(ptr int64 long) winevulkan.vkResetDescriptorPool
@ stdcall vkResetEvent(ptr int64) winevulkan.vkResetEvent
@ stdcall vkResetFences(ptr long ptr) winevulkan.vkResetFences
@ stdcall vkResetQueryPool(ptr int64 long long) winevulkan.vkResetQueryPool
@ stdcall vkSetEvent(ptr int64) winevulkan.vkSetEvent
@ stdcall vkSetPrivateData(ptr long int64 int64 int64) winevulkan.vkSetPrivateData
@ stdcall vkSignalSemaphore(ptr ptr) winevulkan.vkSignalSemaphore
@ stdcall vkTrimCommandPool(ptr int64 long) winevulkan.vkTrimCommandPool
@ stdcall vkUnmapMemory(ptr int64) winevulkan.vkUnmapMemory
@ stdcall vkUpdateDescriptorSetWithTemplate(ptr int64 int64 ptr) winevulkan.vkUpdateDescriptorSetWithTemplate
@ stdcall vkUpdateDescriptorSets(ptr long ptr long ptr) winevulkan.vkUpdateDescriptorSets
@ stdcall vkWaitForFences(ptr long ptr long int64) winevulkan.vkWaitForFences
@ stdcall vkWaitSemaphores(ptr ptr int64) winevulkan.vkWaitSemaphores
