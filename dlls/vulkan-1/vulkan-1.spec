# Automatically generated from Vulkan vk.xml; DO NOT EDIT!
#
# This file is generated from Vulkan vk.xml file covered
# by the following copyright and permission notice:
#
# Copyright 2015-2021 The Khronos Group Inc.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

@ stdcall vkAcquireNextImage2KHR(ptr ptr ptr) winevulkan.wine_vkAcquireNextImage2KHR
@ stdcall vkAcquireNextImageKHR(ptr int64 int64 int64 int64 ptr) winevulkan.wine_vkAcquireNextImageKHR
@ stdcall vkAllocateCommandBuffers(ptr ptr ptr) winevulkan.wine_vkAllocateCommandBuffers
@ stdcall vkAllocateDescriptorSets(ptr ptr ptr) winevulkan.wine_vkAllocateDescriptorSets
@ stdcall vkAllocateMemory(ptr ptr ptr ptr) winevulkan.wine_vkAllocateMemory
@ stdcall vkBeginCommandBuffer(ptr ptr) winevulkan.wine_vkBeginCommandBuffer
@ stdcall vkBindBufferMemory(ptr int64 int64 int64) winevulkan.wine_vkBindBufferMemory
@ stdcall vkBindBufferMemory2(ptr long ptr) winevulkan.wine_vkBindBufferMemory2
@ stdcall vkBindImageMemory(ptr int64 int64 int64) winevulkan.wine_vkBindImageMemory
@ stdcall vkBindImageMemory2(ptr long ptr) winevulkan.wine_vkBindImageMemory2
@ stdcall vkCmdBeginQuery(ptr int64 long long) winevulkan.wine_vkCmdBeginQuery
@ stdcall vkCmdBeginRenderPass(ptr ptr long) winevulkan.wine_vkCmdBeginRenderPass
@ stdcall vkCmdBeginRenderPass2(ptr ptr ptr) winevulkan.wine_vkCmdBeginRenderPass2
@ stdcall vkCmdBindDescriptorSets(ptr long int64 long long ptr long ptr) winevulkan.wine_vkCmdBindDescriptorSets
@ stdcall vkCmdBindIndexBuffer(ptr int64 int64 long) winevulkan.wine_vkCmdBindIndexBuffer
@ stdcall vkCmdBindPipeline(ptr long int64) winevulkan.wine_vkCmdBindPipeline
@ stdcall vkCmdBindVertexBuffers(ptr long long ptr ptr) winevulkan.wine_vkCmdBindVertexBuffers
@ stdcall vkCmdBlitImage(ptr int64 long int64 long long ptr long) winevulkan.wine_vkCmdBlitImage
@ stdcall vkCmdClearAttachments(ptr long ptr long ptr) winevulkan.wine_vkCmdClearAttachments
@ stdcall vkCmdClearColorImage(ptr int64 long ptr long ptr) winevulkan.wine_vkCmdClearColorImage
@ stdcall vkCmdClearDepthStencilImage(ptr int64 long ptr long ptr) winevulkan.wine_vkCmdClearDepthStencilImage
@ stdcall vkCmdCopyBuffer(ptr int64 int64 long ptr) winevulkan.wine_vkCmdCopyBuffer
@ stdcall vkCmdCopyBufferToImage(ptr int64 int64 long long ptr) winevulkan.wine_vkCmdCopyBufferToImage
@ stdcall vkCmdCopyImage(ptr int64 long int64 long long ptr) winevulkan.wine_vkCmdCopyImage
@ stdcall vkCmdCopyImageToBuffer(ptr int64 long int64 long ptr) winevulkan.wine_vkCmdCopyImageToBuffer
@ stdcall vkCmdCopyQueryPoolResults(ptr int64 long long int64 int64 int64 long) winevulkan.wine_vkCmdCopyQueryPoolResults
@ stdcall vkCmdDispatch(ptr long long long) winevulkan.wine_vkCmdDispatch
@ stdcall vkCmdDispatchBase(ptr long long long long long long) winevulkan.wine_vkCmdDispatchBase
@ stdcall vkCmdDispatchIndirect(ptr int64 int64) winevulkan.wine_vkCmdDispatchIndirect
@ stdcall vkCmdDraw(ptr long long long long) winevulkan.wine_vkCmdDraw
@ stdcall vkCmdDrawIndexed(ptr long long long long long) winevulkan.wine_vkCmdDrawIndexed
@ stdcall vkCmdDrawIndexedIndirect(ptr int64 int64 long long) winevulkan.wine_vkCmdDrawIndexedIndirect
@ stdcall vkCmdDrawIndexedIndirectCount(ptr int64 int64 int64 int64 long long) winevulkan.wine_vkCmdDrawIndexedIndirectCount
@ stdcall vkCmdDrawIndirect(ptr int64 int64 long long) winevulkan.wine_vkCmdDrawIndirect
@ stdcall vkCmdDrawIndirectCount(ptr int64 int64 int64 int64 long long) winevulkan.wine_vkCmdDrawIndirectCount
@ stdcall vkCmdEndQuery(ptr int64 long) winevulkan.wine_vkCmdEndQuery
@ stdcall vkCmdEndRenderPass(ptr) winevulkan.wine_vkCmdEndRenderPass
@ stdcall vkCmdEndRenderPass2(ptr ptr) winevulkan.wine_vkCmdEndRenderPass2
@ stdcall vkCmdExecuteCommands(ptr long ptr) winevulkan.wine_vkCmdExecuteCommands
@ stdcall vkCmdFillBuffer(ptr int64 int64 int64 long) winevulkan.wine_vkCmdFillBuffer
@ stdcall vkCmdNextSubpass(ptr long) winevulkan.wine_vkCmdNextSubpass
@ stdcall vkCmdNextSubpass2(ptr ptr ptr) winevulkan.wine_vkCmdNextSubpass2
@ stdcall vkCmdPipelineBarrier(ptr long long long long ptr long ptr long ptr) winevulkan.wine_vkCmdPipelineBarrier
@ stdcall vkCmdPushConstants(ptr int64 long long long ptr) winevulkan.wine_vkCmdPushConstants
@ stdcall vkCmdResetEvent(ptr int64 long) winevulkan.wine_vkCmdResetEvent
@ stdcall vkCmdResetQueryPool(ptr int64 long long) winevulkan.wine_vkCmdResetQueryPool
@ stdcall vkCmdResolveImage(ptr int64 long int64 long long ptr) winevulkan.wine_vkCmdResolveImage
@ stdcall vkCmdSetBlendConstants(ptr ptr) winevulkan.wine_vkCmdSetBlendConstants
@ stdcall vkCmdSetDepthBias(ptr float float float) winevulkan.wine_vkCmdSetDepthBias
@ stdcall vkCmdSetDepthBounds(ptr float float) winevulkan.wine_vkCmdSetDepthBounds
@ stdcall vkCmdSetDeviceMask(ptr long) winevulkan.wine_vkCmdSetDeviceMask
@ stdcall vkCmdSetEvent(ptr int64 long) winevulkan.wine_vkCmdSetEvent
@ stdcall vkCmdSetLineWidth(ptr float) winevulkan.wine_vkCmdSetLineWidth
@ stdcall vkCmdSetScissor(ptr long long ptr) winevulkan.wine_vkCmdSetScissor
@ stdcall vkCmdSetStencilCompareMask(ptr long long) winevulkan.wine_vkCmdSetStencilCompareMask
@ stdcall vkCmdSetStencilReference(ptr long long) winevulkan.wine_vkCmdSetStencilReference
@ stdcall vkCmdSetStencilWriteMask(ptr long long) winevulkan.wine_vkCmdSetStencilWriteMask
@ stdcall vkCmdSetViewport(ptr long long ptr) winevulkan.wine_vkCmdSetViewport
@ stdcall vkCmdUpdateBuffer(ptr int64 int64 int64 ptr) winevulkan.wine_vkCmdUpdateBuffer
@ stdcall vkCmdWaitEvents(ptr long ptr long long long ptr long ptr long ptr) winevulkan.wine_vkCmdWaitEvents
@ stdcall vkCmdWriteTimestamp(ptr long int64 long) winevulkan.wine_vkCmdWriteTimestamp
@ stdcall vkCreateBuffer(ptr ptr ptr ptr) winevulkan.wine_vkCreateBuffer
@ stdcall vkCreateBufferView(ptr ptr ptr ptr) winevulkan.wine_vkCreateBufferView
@ stdcall vkCreateCommandPool(ptr ptr ptr ptr) winevulkan.wine_vkCreateCommandPool
@ stdcall vkCreateComputePipelines(ptr int64 long ptr ptr ptr) winevulkan.wine_vkCreateComputePipelines
@ stdcall vkCreateDescriptorPool(ptr ptr ptr ptr) winevulkan.wine_vkCreateDescriptorPool
@ stdcall vkCreateDescriptorSetLayout(ptr ptr ptr ptr) winevulkan.wine_vkCreateDescriptorSetLayout
@ stdcall vkCreateDescriptorUpdateTemplate(ptr ptr ptr ptr) winevulkan.wine_vkCreateDescriptorUpdateTemplate
@ stdcall vkCreateDevice(ptr ptr ptr ptr) winevulkan.wine_vkCreateDevice
@ stub vkCreateDisplayModeKHR
@ stub vkCreateDisplayPlaneSurfaceKHR
@ stdcall vkCreateEvent(ptr ptr ptr ptr) winevulkan.wine_vkCreateEvent
@ stdcall vkCreateFence(ptr ptr ptr ptr) winevulkan.wine_vkCreateFence
@ stdcall vkCreateFramebuffer(ptr ptr ptr ptr) winevulkan.wine_vkCreateFramebuffer
@ stdcall vkCreateGraphicsPipelines(ptr int64 long ptr ptr ptr) winevulkan.wine_vkCreateGraphicsPipelines
@ stdcall vkCreateImage(ptr ptr ptr ptr) winevulkan.wine_vkCreateImage
@ stdcall vkCreateImageView(ptr ptr ptr ptr) winevulkan.wine_vkCreateImageView
@ stdcall vkCreateInstance(ptr ptr ptr) winevulkan.wine_vkCreateInstance
@ stdcall vkCreatePipelineCache(ptr ptr ptr ptr) winevulkan.wine_vkCreatePipelineCache
@ stdcall vkCreatePipelineLayout(ptr ptr ptr ptr) winevulkan.wine_vkCreatePipelineLayout
@ stdcall vkCreateQueryPool(ptr ptr ptr ptr) winevulkan.wine_vkCreateQueryPool
@ stdcall vkCreateRenderPass(ptr ptr ptr ptr) winevulkan.wine_vkCreateRenderPass
@ stdcall vkCreateRenderPass2(ptr ptr ptr ptr) winevulkan.wine_vkCreateRenderPass2
@ stdcall vkCreateSampler(ptr ptr ptr ptr) winevulkan.wine_vkCreateSampler
@ stdcall vkCreateSamplerYcbcrConversion(ptr ptr ptr ptr) winevulkan.wine_vkCreateSamplerYcbcrConversion
@ stdcall vkCreateSemaphore(ptr ptr ptr ptr) winevulkan.wine_vkCreateSemaphore
@ stdcall vkCreateShaderModule(ptr ptr ptr ptr) winevulkan.wine_vkCreateShaderModule
@ stub vkCreateSharedSwapchainsKHR
@ stdcall vkCreateSwapchainKHR(ptr ptr ptr ptr) winevulkan.wine_vkCreateSwapchainKHR
@ stdcall vkCreateWin32SurfaceKHR(ptr ptr ptr ptr) winevulkan.wine_vkCreateWin32SurfaceKHR
@ stdcall vkDestroyBuffer(ptr int64 ptr) winevulkan.wine_vkDestroyBuffer
@ stdcall vkDestroyBufferView(ptr int64 ptr) winevulkan.wine_vkDestroyBufferView
@ stdcall vkDestroyCommandPool(ptr int64 ptr) winevulkan.wine_vkDestroyCommandPool
@ stdcall vkDestroyDescriptorPool(ptr int64 ptr) winevulkan.wine_vkDestroyDescriptorPool
@ stdcall vkDestroyDescriptorSetLayout(ptr int64 ptr) winevulkan.wine_vkDestroyDescriptorSetLayout
@ stdcall vkDestroyDescriptorUpdateTemplate(ptr int64 ptr) winevulkan.wine_vkDestroyDescriptorUpdateTemplate
@ stdcall vkDestroyDevice(ptr ptr) winevulkan.wine_vkDestroyDevice
@ stdcall vkDestroyEvent(ptr int64 ptr) winevulkan.wine_vkDestroyEvent
@ stdcall vkDestroyFence(ptr int64 ptr) winevulkan.wine_vkDestroyFence
@ stdcall vkDestroyFramebuffer(ptr int64 ptr) winevulkan.wine_vkDestroyFramebuffer
@ stdcall vkDestroyImage(ptr int64 ptr) winevulkan.wine_vkDestroyImage
@ stdcall vkDestroyImageView(ptr int64 ptr) winevulkan.wine_vkDestroyImageView
@ stdcall vkDestroyInstance(ptr ptr) winevulkan.wine_vkDestroyInstance
@ stdcall vkDestroyPipeline(ptr int64 ptr) winevulkan.wine_vkDestroyPipeline
@ stdcall vkDestroyPipelineCache(ptr int64 ptr) winevulkan.wine_vkDestroyPipelineCache
@ stdcall vkDestroyPipelineLayout(ptr int64 ptr) winevulkan.wine_vkDestroyPipelineLayout
@ stdcall vkDestroyQueryPool(ptr int64 ptr) winevulkan.wine_vkDestroyQueryPool
@ stdcall vkDestroyRenderPass(ptr int64 ptr) winevulkan.wine_vkDestroyRenderPass
@ stdcall vkDestroySampler(ptr int64 ptr) winevulkan.wine_vkDestroySampler
@ stdcall vkDestroySamplerYcbcrConversion(ptr int64 ptr) winevulkan.wine_vkDestroySamplerYcbcrConversion
@ stdcall vkDestroySemaphore(ptr int64 ptr) winevulkan.wine_vkDestroySemaphore
@ stdcall vkDestroyShaderModule(ptr int64 ptr) winevulkan.wine_vkDestroyShaderModule
@ stdcall vkDestroySurfaceKHR(ptr int64 ptr) winevulkan.wine_vkDestroySurfaceKHR
@ stdcall vkDestroySwapchainKHR(ptr int64 ptr) winevulkan.wine_vkDestroySwapchainKHR
@ stdcall vkDeviceWaitIdle(ptr) winevulkan.wine_vkDeviceWaitIdle
@ stdcall vkEndCommandBuffer(ptr) winevulkan.wine_vkEndCommandBuffer
@ stdcall vkEnumerateDeviceExtensionProperties(ptr str ptr ptr) winevulkan.wine_vkEnumerateDeviceExtensionProperties
@ stdcall vkEnumerateDeviceLayerProperties(ptr ptr ptr) winevulkan.wine_vkEnumerateDeviceLayerProperties
@ stdcall vkEnumerateInstanceExtensionProperties(str ptr ptr) winevulkan.wine_vkEnumerateInstanceExtensionProperties
@ stdcall vkEnumerateInstanceLayerProperties(ptr ptr) winevulkan.wine_vkEnumerateInstanceLayerProperties
@ stdcall vkEnumerateInstanceVersion(ptr) winevulkan.wine_vkEnumerateInstanceVersion
@ stdcall vkEnumeratePhysicalDeviceGroups(ptr ptr ptr) winevulkan.wine_vkEnumeratePhysicalDeviceGroups
@ stdcall vkEnumeratePhysicalDevices(ptr ptr ptr) winevulkan.wine_vkEnumeratePhysicalDevices
@ stdcall vkFlushMappedMemoryRanges(ptr long ptr) winevulkan.wine_vkFlushMappedMemoryRanges
@ stdcall vkFreeCommandBuffers(ptr int64 long ptr) winevulkan.wine_vkFreeCommandBuffers
@ stdcall vkFreeDescriptorSets(ptr int64 long ptr) winevulkan.wine_vkFreeDescriptorSets
@ stdcall vkFreeMemory(ptr int64 ptr) winevulkan.wine_vkFreeMemory
@ stdcall vkGetBufferDeviceAddress(ptr ptr) winevulkan.wine_vkGetBufferDeviceAddress
@ stdcall vkGetBufferMemoryRequirements(ptr int64 ptr) winevulkan.wine_vkGetBufferMemoryRequirements
@ stdcall vkGetBufferMemoryRequirements2(ptr ptr ptr) winevulkan.wine_vkGetBufferMemoryRequirements2
@ stdcall vkGetBufferOpaqueCaptureAddress(ptr ptr) winevulkan.wine_vkGetBufferOpaqueCaptureAddress
@ stdcall vkGetDescriptorSetLayoutSupport(ptr ptr ptr) winevulkan.wine_vkGetDescriptorSetLayoutSupport
@ stdcall vkGetDeviceGroupPeerMemoryFeatures(ptr long long long ptr) winevulkan.wine_vkGetDeviceGroupPeerMemoryFeatures
@ stdcall vkGetDeviceGroupPresentCapabilitiesKHR(ptr ptr) winevulkan.wine_vkGetDeviceGroupPresentCapabilitiesKHR
@ stdcall vkGetDeviceGroupSurfacePresentModesKHR(ptr int64 ptr) winevulkan.wine_vkGetDeviceGroupSurfacePresentModesKHR
@ stdcall vkGetDeviceMemoryCommitment(ptr int64 ptr) winevulkan.wine_vkGetDeviceMemoryCommitment
@ stdcall vkGetDeviceMemoryOpaqueCaptureAddress(ptr ptr) winevulkan.wine_vkGetDeviceMemoryOpaqueCaptureAddress
@ stdcall vkGetDeviceProcAddr(ptr str) winevulkan.wine_vkGetDeviceProcAddr
@ stdcall vkGetDeviceQueue(ptr long long ptr) winevulkan.wine_vkGetDeviceQueue
@ stdcall vkGetDeviceQueue2(ptr ptr ptr) winevulkan.wine_vkGetDeviceQueue2
@ stub vkGetDisplayModePropertiesKHR
@ stub vkGetDisplayPlaneCapabilitiesKHR
@ stub vkGetDisplayPlaneSupportedDisplaysKHR
@ stdcall vkGetEventStatus(ptr int64) winevulkan.wine_vkGetEventStatus
@ stdcall vkGetFenceStatus(ptr int64) winevulkan.wine_vkGetFenceStatus
@ stdcall vkGetImageMemoryRequirements(ptr int64 ptr) winevulkan.wine_vkGetImageMemoryRequirements
@ stdcall vkGetImageMemoryRequirements2(ptr ptr ptr) winevulkan.wine_vkGetImageMemoryRequirements2
@ stdcall vkGetImageSparseMemoryRequirements(ptr int64 ptr ptr) winevulkan.wine_vkGetImageSparseMemoryRequirements
@ stdcall vkGetImageSparseMemoryRequirements2(ptr ptr ptr ptr) winevulkan.wine_vkGetImageSparseMemoryRequirements2
@ stdcall vkGetImageSubresourceLayout(ptr int64 ptr ptr) winevulkan.wine_vkGetImageSubresourceLayout
@ stdcall vkGetInstanceProcAddr(ptr str) winevulkan.wine_vkGetInstanceProcAddr
@ stub vkGetPhysicalDeviceDisplayPlanePropertiesKHR
@ stub vkGetPhysicalDeviceDisplayPropertiesKHR
@ stdcall vkGetPhysicalDeviceExternalBufferProperties(ptr ptr ptr) winevulkan.wine_vkGetPhysicalDeviceExternalBufferProperties
@ stdcall vkGetPhysicalDeviceExternalFenceProperties(ptr ptr ptr) winevulkan.wine_vkGetPhysicalDeviceExternalFenceProperties
@ stdcall vkGetPhysicalDeviceExternalSemaphoreProperties(ptr ptr ptr) winevulkan.wine_vkGetPhysicalDeviceExternalSemaphoreProperties
@ stdcall vkGetPhysicalDeviceFeatures(ptr ptr) winevulkan.wine_vkGetPhysicalDeviceFeatures
@ stdcall vkGetPhysicalDeviceFeatures2(ptr ptr) winevulkan.wine_vkGetPhysicalDeviceFeatures2
@ stdcall vkGetPhysicalDeviceFormatProperties(ptr long ptr) winevulkan.wine_vkGetPhysicalDeviceFormatProperties
@ stdcall vkGetPhysicalDeviceFormatProperties2(ptr long ptr) winevulkan.wine_vkGetPhysicalDeviceFormatProperties2
@ stdcall vkGetPhysicalDeviceImageFormatProperties(ptr long long long long long ptr) winevulkan.wine_vkGetPhysicalDeviceImageFormatProperties
@ stdcall vkGetPhysicalDeviceImageFormatProperties2(ptr ptr ptr) winevulkan.wine_vkGetPhysicalDeviceImageFormatProperties2
@ stdcall vkGetPhysicalDeviceMemoryProperties(ptr ptr) winevulkan.wine_vkGetPhysicalDeviceMemoryProperties
@ stdcall vkGetPhysicalDeviceMemoryProperties2(ptr ptr) winevulkan.wine_vkGetPhysicalDeviceMemoryProperties2
@ stdcall vkGetPhysicalDevicePresentRectanglesKHR(ptr int64 ptr ptr) winevulkan.wine_vkGetPhysicalDevicePresentRectanglesKHR
@ stdcall vkGetPhysicalDeviceProperties(ptr ptr) winevulkan.wine_vkGetPhysicalDeviceProperties
@ stdcall vkGetPhysicalDeviceProperties2(ptr ptr) winevulkan.wine_vkGetPhysicalDeviceProperties2
@ stdcall vkGetPhysicalDeviceQueueFamilyProperties(ptr ptr ptr) winevulkan.wine_vkGetPhysicalDeviceQueueFamilyProperties
@ stdcall vkGetPhysicalDeviceQueueFamilyProperties2(ptr ptr ptr) winevulkan.wine_vkGetPhysicalDeviceQueueFamilyProperties2
@ stdcall vkGetPhysicalDeviceSparseImageFormatProperties(ptr long long long long long ptr ptr) winevulkan.wine_vkGetPhysicalDeviceSparseImageFormatProperties
@ stdcall vkGetPhysicalDeviceSparseImageFormatProperties2(ptr ptr ptr ptr) winevulkan.wine_vkGetPhysicalDeviceSparseImageFormatProperties2
@ stdcall vkGetPhysicalDeviceSurfaceCapabilities2KHR(ptr ptr ptr) winevulkan.wine_vkGetPhysicalDeviceSurfaceCapabilities2KHR
@ stdcall vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ptr int64 ptr) winevulkan.wine_vkGetPhysicalDeviceSurfaceCapabilitiesKHR
@ stdcall vkGetPhysicalDeviceSurfaceFormats2KHR(ptr ptr ptr ptr) winevulkan.wine_vkGetPhysicalDeviceSurfaceFormats2KHR
@ stdcall vkGetPhysicalDeviceSurfaceFormatsKHR(ptr int64 ptr ptr) winevulkan.wine_vkGetPhysicalDeviceSurfaceFormatsKHR
@ stdcall vkGetPhysicalDeviceSurfacePresentModesKHR(ptr int64 ptr ptr) winevulkan.wine_vkGetPhysicalDeviceSurfacePresentModesKHR
@ stdcall vkGetPhysicalDeviceSurfaceSupportKHR(ptr long int64 ptr) winevulkan.wine_vkGetPhysicalDeviceSurfaceSupportKHR
@ stdcall vkGetPhysicalDeviceWin32PresentationSupportKHR(ptr long) winevulkan.wine_vkGetPhysicalDeviceWin32PresentationSupportKHR
@ stdcall vkGetPipelineCacheData(ptr int64 ptr ptr) winevulkan.wine_vkGetPipelineCacheData
@ stdcall vkGetQueryPoolResults(ptr int64 long long long ptr int64 long) winevulkan.wine_vkGetQueryPoolResults
@ stdcall vkGetRenderAreaGranularity(ptr int64 ptr) winevulkan.wine_vkGetRenderAreaGranularity
@ stdcall vkGetSemaphoreCounterValue(ptr int64 ptr) winevulkan.wine_vkGetSemaphoreCounterValue
@ stdcall vkGetSwapchainImagesKHR(ptr int64 ptr ptr) winevulkan.wine_vkGetSwapchainImagesKHR
@ stdcall vkInvalidateMappedMemoryRanges(ptr long ptr) winevulkan.wine_vkInvalidateMappedMemoryRanges
@ stdcall vkMapMemory(ptr int64 int64 int64 long ptr) winevulkan.wine_vkMapMemory
@ stdcall vkMergePipelineCaches(ptr int64 long ptr) winevulkan.wine_vkMergePipelineCaches
@ stdcall vkQueueBindSparse(ptr long ptr int64) winevulkan.wine_vkQueueBindSparse
@ stdcall vkQueuePresentKHR(ptr ptr) winevulkan.wine_vkQueuePresentKHR
@ stdcall vkQueueSubmit(ptr long ptr int64) winevulkan.wine_vkQueueSubmit
@ stdcall vkQueueWaitIdle(ptr) winevulkan.wine_vkQueueWaitIdle
@ stdcall vkResetCommandBuffer(ptr long) winevulkan.wine_vkResetCommandBuffer
@ stdcall vkResetCommandPool(ptr int64 long) winevulkan.wine_vkResetCommandPool
@ stdcall vkResetDescriptorPool(ptr int64 long) winevulkan.wine_vkResetDescriptorPool
@ stdcall vkResetEvent(ptr int64) winevulkan.wine_vkResetEvent
@ stdcall vkResetFences(ptr long ptr) winevulkan.wine_vkResetFences
@ stdcall vkResetQueryPool(ptr int64 long long) winevulkan.wine_vkResetQueryPool
@ stdcall vkSetEvent(ptr int64) winevulkan.wine_vkSetEvent
@ stdcall vkSignalSemaphore(ptr ptr) winevulkan.wine_vkSignalSemaphore
@ stdcall vkTrimCommandPool(ptr int64 long) winevulkan.wine_vkTrimCommandPool
@ stdcall vkUnmapMemory(ptr int64) winevulkan.wine_vkUnmapMemory
@ stdcall vkUpdateDescriptorSetWithTemplate(ptr int64 int64 ptr) winevulkan.wine_vkUpdateDescriptorSetWithTemplate
@ stdcall vkUpdateDescriptorSets(ptr long ptr long ptr) winevulkan.wine_vkUpdateDescriptorSets
@ stdcall vkWaitForFences(ptr long ptr long int64) winevulkan.wine_vkWaitForFences
@ stdcall vkWaitSemaphores(ptr ptr int64) winevulkan.wine_vkWaitSemaphores
