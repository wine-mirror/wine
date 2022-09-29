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

#include "vulkan_loader.h"

WINE_DEFAULT_DEBUG_CHANNEL(vulkan);

VkResult WINAPI vkAcquireNextImage2KHR(VkDevice device, const VkAcquireNextImageInfoKHR *pAcquireInfo, uint32_t *pImageIndex)
{
    struct vkAcquireNextImage2KHR_params params;
    params.device = device;
    params.pAcquireInfo = pAcquireInfo;
    params.pImageIndex = pImageIndex;
    vk_unix_call(unix_vkAcquireNextImage2KHR, &params);
    return params.result;
}

VkResult WINAPI vkAcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t *pImageIndex)
{
    struct vkAcquireNextImageKHR_params params;
    params.device = device;
    params.swapchain = swapchain;
    params.timeout = timeout;
    params.semaphore = semaphore;
    params.fence = fence;
    params.pImageIndex = pImageIndex;
    vk_unix_call(unix_vkAcquireNextImageKHR, &params);
    return params.result;
}

VkResult WINAPI vkAcquirePerformanceConfigurationINTEL(VkDevice device, const VkPerformanceConfigurationAcquireInfoINTEL *pAcquireInfo, VkPerformanceConfigurationINTEL *pConfiguration)
{
    struct vkAcquirePerformanceConfigurationINTEL_params params;
    params.device = device;
    params.pAcquireInfo = pAcquireInfo;
    params.pConfiguration = pConfiguration;
    vk_unix_call(unix_vkAcquirePerformanceConfigurationINTEL, &params);
    return params.result;
}

VkResult WINAPI vkAcquireProfilingLockKHR(VkDevice device, const VkAcquireProfilingLockInfoKHR *pInfo)
{
    struct vkAcquireProfilingLockKHR_params params;
    params.device = device;
    params.pInfo = pInfo;
    vk_unix_call(unix_vkAcquireProfilingLockKHR, &params);
    return params.result;
}

VkResult WINAPI vkAllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo *pAllocateInfo, VkDescriptorSet *pDescriptorSets)
{
    struct vkAllocateDescriptorSets_params params;
    params.device = device;
    params.pAllocateInfo = pAllocateInfo;
    params.pDescriptorSets = pDescriptorSets;
    vk_unix_call(unix_vkAllocateDescriptorSets, &params);
    return params.result;
}

VkResult WINAPI vkAllocateMemory(VkDevice device, const VkMemoryAllocateInfo *pAllocateInfo, const VkAllocationCallbacks *pAllocator, VkDeviceMemory *pMemory)
{
    struct vkAllocateMemory_params params;
    params.device = device;
    params.pAllocateInfo = pAllocateInfo;
    params.pAllocator = pAllocator;
    params.pMemory = pMemory;
    vk_unix_call(unix_vkAllocateMemory, &params);
    return params.result;
}

VkResult WINAPI vkBeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo *pBeginInfo)
{
    struct vkBeginCommandBuffer_params params;
    params.commandBuffer = commandBuffer;
    params.pBeginInfo = pBeginInfo;
    vk_unix_call(unix_vkBeginCommandBuffer, &params);
    return params.result;
}

VkResult WINAPI vkBindAccelerationStructureMemoryNV(VkDevice device, uint32_t bindInfoCount, const VkBindAccelerationStructureMemoryInfoNV *pBindInfos)
{
    struct vkBindAccelerationStructureMemoryNV_params params;
    params.device = device;
    params.bindInfoCount = bindInfoCount;
    params.pBindInfos = pBindInfos;
    vk_unix_call(unix_vkBindAccelerationStructureMemoryNV, &params);
    return params.result;
}

VkResult WINAPI vkBindBufferMemory(VkDevice device, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset)
{
    struct vkBindBufferMemory_params params;
    params.device = device;
    params.buffer = buffer;
    params.memory = memory;
    params.memoryOffset = memoryOffset;
    vk_unix_call(unix_vkBindBufferMemory, &params);
    return params.result;
}

VkResult WINAPI vkBindBufferMemory2(VkDevice device, uint32_t bindInfoCount, const VkBindBufferMemoryInfo *pBindInfos)
{
    struct vkBindBufferMemory2_params params;
    params.device = device;
    params.bindInfoCount = bindInfoCount;
    params.pBindInfos = pBindInfos;
    vk_unix_call(unix_vkBindBufferMemory2, &params);
    return params.result;
}

VkResult WINAPI vkBindBufferMemory2KHR(VkDevice device, uint32_t bindInfoCount, const VkBindBufferMemoryInfo *pBindInfos)
{
    struct vkBindBufferMemory2KHR_params params;
    params.device = device;
    params.bindInfoCount = bindInfoCount;
    params.pBindInfos = pBindInfos;
    vk_unix_call(unix_vkBindBufferMemory2KHR, &params);
    return params.result;
}

VkResult WINAPI vkBindImageMemory(VkDevice device, VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset)
{
    struct vkBindImageMemory_params params;
    params.device = device;
    params.image = image;
    params.memory = memory;
    params.memoryOffset = memoryOffset;
    vk_unix_call(unix_vkBindImageMemory, &params);
    return params.result;
}

VkResult WINAPI vkBindImageMemory2(VkDevice device, uint32_t bindInfoCount, const VkBindImageMemoryInfo *pBindInfos)
{
    struct vkBindImageMemory2_params params;
    params.device = device;
    params.bindInfoCount = bindInfoCount;
    params.pBindInfos = pBindInfos;
    vk_unix_call(unix_vkBindImageMemory2, &params);
    return params.result;
}

VkResult WINAPI vkBindImageMemory2KHR(VkDevice device, uint32_t bindInfoCount, const VkBindImageMemoryInfo *pBindInfos)
{
    struct vkBindImageMemory2KHR_params params;
    params.device = device;
    params.bindInfoCount = bindInfoCount;
    params.pBindInfos = pBindInfos;
    vk_unix_call(unix_vkBindImageMemory2KHR, &params);
    return params.result;
}

VkResult WINAPI vkBindOpticalFlowSessionImageNV(VkDevice device, VkOpticalFlowSessionNV session, VkOpticalFlowSessionBindingPointNV bindingPoint, VkImageView view, VkImageLayout layout)
{
    struct vkBindOpticalFlowSessionImageNV_params params;
    params.device = device;
    params.session = session;
    params.bindingPoint = bindingPoint;
    params.view = view;
    params.layout = layout;
    vk_unix_call(unix_vkBindOpticalFlowSessionImageNV, &params);
    return params.result;
}

VkResult WINAPI vkBuildAccelerationStructuresKHR(VkDevice device, VkDeferredOperationKHR deferredOperation, uint32_t infoCount, const VkAccelerationStructureBuildGeometryInfoKHR *pInfos, const VkAccelerationStructureBuildRangeInfoKHR * const*ppBuildRangeInfos)
{
    struct vkBuildAccelerationStructuresKHR_params params;
    params.device = device;
    params.deferredOperation = deferredOperation;
    params.infoCount = infoCount;
    params.pInfos = pInfos;
    params.ppBuildRangeInfos = ppBuildRangeInfos;
    vk_unix_call(unix_vkBuildAccelerationStructuresKHR, &params);
    return params.result;
}

VkResult WINAPI vkBuildMicromapsEXT(VkDevice device, VkDeferredOperationKHR deferredOperation, uint32_t infoCount, const VkMicromapBuildInfoEXT *pInfos)
{
    struct vkBuildMicromapsEXT_params params;
    params.device = device;
    params.deferredOperation = deferredOperation;
    params.infoCount = infoCount;
    params.pInfos = pInfos;
    vk_unix_call(unix_vkBuildMicromapsEXT, &params);
    return params.result;
}

void WINAPI vkCmdBeginConditionalRenderingEXT(VkCommandBuffer commandBuffer, const VkConditionalRenderingBeginInfoEXT *pConditionalRenderingBegin)
{
    struct vkCmdBeginConditionalRenderingEXT_params params;
    params.commandBuffer = commandBuffer;
    params.pConditionalRenderingBegin = pConditionalRenderingBegin;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdBeginConditionalRenderingEXT, &params);
}

void WINAPI vkCmdBeginDebugUtilsLabelEXT(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT *pLabelInfo)
{
    struct vkCmdBeginDebugUtilsLabelEXT_params params;
    params.commandBuffer = commandBuffer;
    params.pLabelInfo = pLabelInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdBeginDebugUtilsLabelEXT, &params);
}

void WINAPI vkCmdBeginQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags)
{
    struct vkCmdBeginQuery_params params;
    params.commandBuffer = commandBuffer;
    params.queryPool = queryPool;
    params.query = query;
    params.flags = flags;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdBeginQuery, &params);
}

void WINAPI vkCmdBeginQueryIndexedEXT(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags, uint32_t index)
{
    struct vkCmdBeginQueryIndexedEXT_params params;
    params.commandBuffer = commandBuffer;
    params.queryPool = queryPool;
    params.query = query;
    params.flags = flags;
    params.index = index;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdBeginQueryIndexedEXT, &params);
}

void WINAPI vkCmdBeginRenderPass(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo *pRenderPassBegin, VkSubpassContents contents)
{
    struct vkCmdBeginRenderPass_params params;
    params.commandBuffer = commandBuffer;
    params.pRenderPassBegin = pRenderPassBegin;
    params.contents = contents;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdBeginRenderPass, &params);
}

void WINAPI vkCmdBeginRenderPass2(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo *pRenderPassBegin, const VkSubpassBeginInfo *pSubpassBeginInfo)
{
    struct vkCmdBeginRenderPass2_params params;
    params.commandBuffer = commandBuffer;
    params.pRenderPassBegin = pRenderPassBegin;
    params.pSubpassBeginInfo = pSubpassBeginInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdBeginRenderPass2, &params);
}

void WINAPI vkCmdBeginRenderPass2KHR(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo *pRenderPassBegin, const VkSubpassBeginInfo *pSubpassBeginInfo)
{
    struct vkCmdBeginRenderPass2KHR_params params;
    params.commandBuffer = commandBuffer;
    params.pRenderPassBegin = pRenderPassBegin;
    params.pSubpassBeginInfo = pSubpassBeginInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdBeginRenderPass2KHR, &params);
}

void WINAPI vkCmdBeginRendering(VkCommandBuffer commandBuffer, const VkRenderingInfo *pRenderingInfo)
{
    struct vkCmdBeginRendering_params params;
    params.commandBuffer = commandBuffer;
    params.pRenderingInfo = pRenderingInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdBeginRendering, &params);
}

void WINAPI vkCmdBeginRenderingKHR(VkCommandBuffer commandBuffer, const VkRenderingInfo *pRenderingInfo)
{
    struct vkCmdBeginRenderingKHR_params params;
    params.commandBuffer = commandBuffer;
    params.pRenderingInfo = pRenderingInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdBeginRenderingKHR, &params);
}

void WINAPI vkCmdBeginTransformFeedbackEXT(VkCommandBuffer commandBuffer, uint32_t firstCounterBuffer, uint32_t counterBufferCount, const VkBuffer *pCounterBuffers, const VkDeviceSize *pCounterBufferOffsets)
{
    struct vkCmdBeginTransformFeedbackEXT_params params;
    params.commandBuffer = commandBuffer;
    params.firstCounterBuffer = firstCounterBuffer;
    params.counterBufferCount = counterBufferCount;
    params.pCounterBuffers = pCounterBuffers;
    params.pCounterBufferOffsets = pCounterBufferOffsets;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdBeginTransformFeedbackEXT, &params);
}

void WINAPI vkCmdBindDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount, const VkDescriptorSet *pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t *pDynamicOffsets)
{
    struct vkCmdBindDescriptorSets_params params;
    params.commandBuffer = commandBuffer;
    params.pipelineBindPoint = pipelineBindPoint;
    params.layout = layout;
    params.firstSet = firstSet;
    params.descriptorSetCount = descriptorSetCount;
    params.pDescriptorSets = pDescriptorSets;
    params.dynamicOffsetCount = dynamicOffsetCount;
    params.pDynamicOffsets = pDynamicOffsets;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdBindDescriptorSets, &params);
}

void WINAPI vkCmdBindIndexBuffer(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType)
{
    struct vkCmdBindIndexBuffer_params params;
    params.commandBuffer = commandBuffer;
    params.buffer = buffer;
    params.offset = offset;
    params.indexType = indexType;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdBindIndexBuffer, &params);
}

void WINAPI vkCmdBindInvocationMaskHUAWEI(VkCommandBuffer commandBuffer, VkImageView imageView, VkImageLayout imageLayout)
{
    struct vkCmdBindInvocationMaskHUAWEI_params params;
    params.commandBuffer = commandBuffer;
    params.imageView = imageView;
    params.imageLayout = imageLayout;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdBindInvocationMaskHUAWEI, &params);
}

void WINAPI vkCmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline)
{
    struct vkCmdBindPipeline_params params;
    params.commandBuffer = commandBuffer;
    params.pipelineBindPoint = pipelineBindPoint;
    params.pipeline = pipeline;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdBindPipeline, &params);
}

void WINAPI vkCmdBindPipelineShaderGroupNV(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline, uint32_t groupIndex)
{
    struct vkCmdBindPipelineShaderGroupNV_params params;
    params.commandBuffer = commandBuffer;
    params.pipelineBindPoint = pipelineBindPoint;
    params.pipeline = pipeline;
    params.groupIndex = groupIndex;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdBindPipelineShaderGroupNV, &params);
}

void WINAPI vkCmdBindShadingRateImageNV(VkCommandBuffer commandBuffer, VkImageView imageView, VkImageLayout imageLayout)
{
    struct vkCmdBindShadingRateImageNV_params params;
    params.commandBuffer = commandBuffer;
    params.imageView = imageView;
    params.imageLayout = imageLayout;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdBindShadingRateImageNV, &params);
}

void WINAPI vkCmdBindTransformFeedbackBuffersEXT(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer *pBuffers, const VkDeviceSize *pOffsets, const VkDeviceSize *pSizes)
{
    struct vkCmdBindTransformFeedbackBuffersEXT_params params;
    params.commandBuffer = commandBuffer;
    params.firstBinding = firstBinding;
    params.bindingCount = bindingCount;
    params.pBuffers = pBuffers;
    params.pOffsets = pOffsets;
    params.pSizes = pSizes;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdBindTransformFeedbackBuffersEXT, &params);
}

void WINAPI vkCmdBindVertexBuffers(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer *pBuffers, const VkDeviceSize *pOffsets)
{
    struct vkCmdBindVertexBuffers_params params;
    params.commandBuffer = commandBuffer;
    params.firstBinding = firstBinding;
    params.bindingCount = bindingCount;
    params.pBuffers = pBuffers;
    params.pOffsets = pOffsets;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdBindVertexBuffers, &params);
}

void WINAPI vkCmdBindVertexBuffers2(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer *pBuffers, const VkDeviceSize *pOffsets, const VkDeviceSize *pSizes, const VkDeviceSize *pStrides)
{
    struct vkCmdBindVertexBuffers2_params params;
    params.commandBuffer = commandBuffer;
    params.firstBinding = firstBinding;
    params.bindingCount = bindingCount;
    params.pBuffers = pBuffers;
    params.pOffsets = pOffsets;
    params.pSizes = pSizes;
    params.pStrides = pStrides;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdBindVertexBuffers2, &params);
}

void WINAPI vkCmdBindVertexBuffers2EXT(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer *pBuffers, const VkDeviceSize *pOffsets, const VkDeviceSize *pSizes, const VkDeviceSize *pStrides)
{
    struct vkCmdBindVertexBuffers2EXT_params params;
    params.commandBuffer = commandBuffer;
    params.firstBinding = firstBinding;
    params.bindingCount = bindingCount;
    params.pBuffers = pBuffers;
    params.pOffsets = pOffsets;
    params.pSizes = pSizes;
    params.pStrides = pStrides;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdBindVertexBuffers2EXT, &params);
}

void WINAPI vkCmdBlitImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit *pRegions, VkFilter filter)
{
    struct vkCmdBlitImage_params params;
    params.commandBuffer = commandBuffer;
    params.srcImage = srcImage;
    params.srcImageLayout = srcImageLayout;
    params.dstImage = dstImage;
    params.dstImageLayout = dstImageLayout;
    params.regionCount = regionCount;
    params.pRegions = pRegions;
    params.filter = filter;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdBlitImage, &params);
}

void WINAPI vkCmdBlitImage2(VkCommandBuffer commandBuffer, const VkBlitImageInfo2 *pBlitImageInfo)
{
    struct vkCmdBlitImage2_params params;
    params.commandBuffer = commandBuffer;
    params.pBlitImageInfo = pBlitImageInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdBlitImage2, &params);
}

void WINAPI vkCmdBlitImage2KHR(VkCommandBuffer commandBuffer, const VkBlitImageInfo2 *pBlitImageInfo)
{
    struct vkCmdBlitImage2KHR_params params;
    params.commandBuffer = commandBuffer;
    params.pBlitImageInfo = pBlitImageInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdBlitImage2KHR, &params);
}

void WINAPI vkCmdBuildAccelerationStructureNV(VkCommandBuffer commandBuffer, const VkAccelerationStructureInfoNV *pInfo, VkBuffer instanceData, VkDeviceSize instanceOffset, VkBool32 update, VkAccelerationStructureNV dst, VkAccelerationStructureNV src, VkBuffer scratch, VkDeviceSize scratchOffset)
{
    struct vkCmdBuildAccelerationStructureNV_params params;
    params.commandBuffer = commandBuffer;
    params.pInfo = pInfo;
    params.instanceData = instanceData;
    params.instanceOffset = instanceOffset;
    params.update = update;
    params.dst = dst;
    params.src = src;
    params.scratch = scratch;
    params.scratchOffset = scratchOffset;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdBuildAccelerationStructureNV, &params);
}

void WINAPI vkCmdBuildAccelerationStructuresIndirectKHR(VkCommandBuffer commandBuffer, uint32_t infoCount, const VkAccelerationStructureBuildGeometryInfoKHR *pInfos, const VkDeviceAddress *pIndirectDeviceAddresses, const uint32_t *pIndirectStrides, const uint32_t * const*ppMaxPrimitiveCounts)
{
    struct vkCmdBuildAccelerationStructuresIndirectKHR_params params;
    params.commandBuffer = commandBuffer;
    params.infoCount = infoCount;
    params.pInfos = pInfos;
    params.pIndirectDeviceAddresses = pIndirectDeviceAddresses;
    params.pIndirectStrides = pIndirectStrides;
    params.ppMaxPrimitiveCounts = ppMaxPrimitiveCounts;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdBuildAccelerationStructuresIndirectKHR, &params);
}

void WINAPI vkCmdBuildAccelerationStructuresKHR(VkCommandBuffer commandBuffer, uint32_t infoCount, const VkAccelerationStructureBuildGeometryInfoKHR *pInfos, const VkAccelerationStructureBuildRangeInfoKHR * const*ppBuildRangeInfos)
{
    struct vkCmdBuildAccelerationStructuresKHR_params params;
    params.commandBuffer = commandBuffer;
    params.infoCount = infoCount;
    params.pInfos = pInfos;
    params.ppBuildRangeInfos = ppBuildRangeInfos;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdBuildAccelerationStructuresKHR, &params);
}

void WINAPI vkCmdBuildMicromapsEXT(VkCommandBuffer commandBuffer, uint32_t infoCount, const VkMicromapBuildInfoEXT *pInfos)
{
    struct vkCmdBuildMicromapsEXT_params params;
    params.commandBuffer = commandBuffer;
    params.infoCount = infoCount;
    params.pInfos = pInfos;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdBuildMicromapsEXT, &params);
}

void WINAPI vkCmdClearAttachments(VkCommandBuffer commandBuffer, uint32_t attachmentCount, const VkClearAttachment *pAttachments, uint32_t rectCount, const VkClearRect *pRects)
{
    struct vkCmdClearAttachments_params params;
    params.commandBuffer = commandBuffer;
    params.attachmentCount = attachmentCount;
    params.pAttachments = pAttachments;
    params.rectCount = rectCount;
    params.pRects = pRects;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdClearAttachments, &params);
}

void WINAPI vkCmdClearColorImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, const VkClearColorValue *pColor, uint32_t rangeCount, const VkImageSubresourceRange *pRanges)
{
    struct vkCmdClearColorImage_params params;
    params.commandBuffer = commandBuffer;
    params.image = image;
    params.imageLayout = imageLayout;
    params.pColor = pColor;
    params.rangeCount = rangeCount;
    params.pRanges = pRanges;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdClearColorImage, &params);
}

void WINAPI vkCmdClearDepthStencilImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, const VkClearDepthStencilValue *pDepthStencil, uint32_t rangeCount, const VkImageSubresourceRange *pRanges)
{
    struct vkCmdClearDepthStencilImage_params params;
    params.commandBuffer = commandBuffer;
    params.image = image;
    params.imageLayout = imageLayout;
    params.pDepthStencil = pDepthStencil;
    params.rangeCount = rangeCount;
    params.pRanges = pRanges;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdClearDepthStencilImage, &params);
}

void WINAPI vkCmdCopyAccelerationStructureKHR(VkCommandBuffer commandBuffer, const VkCopyAccelerationStructureInfoKHR *pInfo)
{
    struct vkCmdCopyAccelerationStructureKHR_params params;
    params.commandBuffer = commandBuffer;
    params.pInfo = pInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdCopyAccelerationStructureKHR, &params);
}

void WINAPI vkCmdCopyAccelerationStructureNV(VkCommandBuffer commandBuffer, VkAccelerationStructureNV dst, VkAccelerationStructureNV src, VkCopyAccelerationStructureModeKHR mode)
{
    struct vkCmdCopyAccelerationStructureNV_params params;
    params.commandBuffer = commandBuffer;
    params.dst = dst;
    params.src = src;
    params.mode = mode;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdCopyAccelerationStructureNV, &params);
}

void WINAPI vkCmdCopyAccelerationStructureToMemoryKHR(VkCommandBuffer commandBuffer, const VkCopyAccelerationStructureToMemoryInfoKHR *pInfo)
{
    struct vkCmdCopyAccelerationStructureToMemoryKHR_params params;
    params.commandBuffer = commandBuffer;
    params.pInfo = pInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdCopyAccelerationStructureToMemoryKHR, &params);
}

void WINAPI vkCmdCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy *pRegions)
{
    struct vkCmdCopyBuffer_params params;
    params.commandBuffer = commandBuffer;
    params.srcBuffer = srcBuffer;
    params.dstBuffer = dstBuffer;
    params.regionCount = regionCount;
    params.pRegions = pRegions;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdCopyBuffer, &params);
}

void WINAPI vkCmdCopyBuffer2(VkCommandBuffer commandBuffer, const VkCopyBufferInfo2 *pCopyBufferInfo)
{
    struct vkCmdCopyBuffer2_params params;
    params.commandBuffer = commandBuffer;
    params.pCopyBufferInfo = pCopyBufferInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdCopyBuffer2, &params);
}

void WINAPI vkCmdCopyBuffer2KHR(VkCommandBuffer commandBuffer, const VkCopyBufferInfo2 *pCopyBufferInfo)
{
    struct vkCmdCopyBuffer2KHR_params params;
    params.commandBuffer = commandBuffer;
    params.pCopyBufferInfo = pCopyBufferInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdCopyBuffer2KHR, &params);
}

void WINAPI vkCmdCopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy *pRegions)
{
    struct vkCmdCopyBufferToImage_params params;
    params.commandBuffer = commandBuffer;
    params.srcBuffer = srcBuffer;
    params.dstImage = dstImage;
    params.dstImageLayout = dstImageLayout;
    params.regionCount = regionCount;
    params.pRegions = pRegions;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdCopyBufferToImage, &params);
}

void WINAPI vkCmdCopyBufferToImage2(VkCommandBuffer commandBuffer, const VkCopyBufferToImageInfo2 *pCopyBufferToImageInfo)
{
    struct vkCmdCopyBufferToImage2_params params;
    params.commandBuffer = commandBuffer;
    params.pCopyBufferToImageInfo = pCopyBufferToImageInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdCopyBufferToImage2, &params);
}

void WINAPI vkCmdCopyBufferToImage2KHR(VkCommandBuffer commandBuffer, const VkCopyBufferToImageInfo2 *pCopyBufferToImageInfo)
{
    struct vkCmdCopyBufferToImage2KHR_params params;
    params.commandBuffer = commandBuffer;
    params.pCopyBufferToImageInfo = pCopyBufferToImageInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdCopyBufferToImage2KHR, &params);
}

void WINAPI vkCmdCopyImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageCopy *pRegions)
{
    struct vkCmdCopyImage_params params;
    params.commandBuffer = commandBuffer;
    params.srcImage = srcImage;
    params.srcImageLayout = srcImageLayout;
    params.dstImage = dstImage;
    params.dstImageLayout = dstImageLayout;
    params.regionCount = regionCount;
    params.pRegions = pRegions;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdCopyImage, &params);
}

void WINAPI vkCmdCopyImage2(VkCommandBuffer commandBuffer, const VkCopyImageInfo2 *pCopyImageInfo)
{
    struct vkCmdCopyImage2_params params;
    params.commandBuffer = commandBuffer;
    params.pCopyImageInfo = pCopyImageInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdCopyImage2, &params);
}

void WINAPI vkCmdCopyImage2KHR(VkCommandBuffer commandBuffer, const VkCopyImageInfo2 *pCopyImageInfo)
{
    struct vkCmdCopyImage2KHR_params params;
    params.commandBuffer = commandBuffer;
    params.pCopyImageInfo = pCopyImageInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdCopyImage2KHR, &params);
}

void WINAPI vkCmdCopyImageToBuffer(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferImageCopy *pRegions)
{
    struct vkCmdCopyImageToBuffer_params params;
    params.commandBuffer = commandBuffer;
    params.srcImage = srcImage;
    params.srcImageLayout = srcImageLayout;
    params.dstBuffer = dstBuffer;
    params.regionCount = regionCount;
    params.pRegions = pRegions;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdCopyImageToBuffer, &params);
}

void WINAPI vkCmdCopyImageToBuffer2(VkCommandBuffer commandBuffer, const VkCopyImageToBufferInfo2 *pCopyImageToBufferInfo)
{
    struct vkCmdCopyImageToBuffer2_params params;
    params.commandBuffer = commandBuffer;
    params.pCopyImageToBufferInfo = pCopyImageToBufferInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdCopyImageToBuffer2, &params);
}

void WINAPI vkCmdCopyImageToBuffer2KHR(VkCommandBuffer commandBuffer, const VkCopyImageToBufferInfo2 *pCopyImageToBufferInfo)
{
    struct vkCmdCopyImageToBuffer2KHR_params params;
    params.commandBuffer = commandBuffer;
    params.pCopyImageToBufferInfo = pCopyImageToBufferInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdCopyImageToBuffer2KHR, &params);
}

void WINAPI vkCmdCopyMemoryToAccelerationStructureKHR(VkCommandBuffer commandBuffer, const VkCopyMemoryToAccelerationStructureInfoKHR *pInfo)
{
    struct vkCmdCopyMemoryToAccelerationStructureKHR_params params;
    params.commandBuffer = commandBuffer;
    params.pInfo = pInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdCopyMemoryToAccelerationStructureKHR, &params);
}

void WINAPI vkCmdCopyMemoryToMicromapEXT(VkCommandBuffer commandBuffer, const VkCopyMemoryToMicromapInfoEXT *pInfo)
{
    struct vkCmdCopyMemoryToMicromapEXT_params params;
    params.commandBuffer = commandBuffer;
    params.pInfo = pInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdCopyMemoryToMicromapEXT, &params);
}

void WINAPI vkCmdCopyMicromapEXT(VkCommandBuffer commandBuffer, const VkCopyMicromapInfoEXT *pInfo)
{
    struct vkCmdCopyMicromapEXT_params params;
    params.commandBuffer = commandBuffer;
    params.pInfo = pInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdCopyMicromapEXT, &params);
}

void WINAPI vkCmdCopyMicromapToMemoryEXT(VkCommandBuffer commandBuffer, const VkCopyMicromapToMemoryInfoEXT *pInfo)
{
    struct vkCmdCopyMicromapToMemoryEXT_params params;
    params.commandBuffer = commandBuffer;
    params.pInfo = pInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdCopyMicromapToMemoryEXT, &params);
}

void WINAPI vkCmdCopyQueryPoolResults(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize stride, VkQueryResultFlags flags)
{
    struct vkCmdCopyQueryPoolResults_params params;
    params.commandBuffer = commandBuffer;
    params.queryPool = queryPool;
    params.firstQuery = firstQuery;
    params.queryCount = queryCount;
    params.dstBuffer = dstBuffer;
    params.dstOffset = dstOffset;
    params.stride = stride;
    params.flags = flags;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdCopyQueryPoolResults, &params);
}

void WINAPI vkCmdCuLaunchKernelNVX(VkCommandBuffer commandBuffer, const VkCuLaunchInfoNVX *pLaunchInfo)
{
    struct vkCmdCuLaunchKernelNVX_params params;
    params.commandBuffer = commandBuffer;
    params.pLaunchInfo = pLaunchInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdCuLaunchKernelNVX, &params);
}

void WINAPI vkCmdDebugMarkerBeginEXT(VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT *pMarkerInfo)
{
    struct vkCmdDebugMarkerBeginEXT_params params;
    params.commandBuffer = commandBuffer;
    params.pMarkerInfo = pMarkerInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdDebugMarkerBeginEXT, &params);
}

void WINAPI vkCmdDebugMarkerEndEXT(VkCommandBuffer commandBuffer)
{
    struct vkCmdDebugMarkerEndEXT_params params;
    params.commandBuffer = commandBuffer;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdDebugMarkerEndEXT, &params);
}

void WINAPI vkCmdDebugMarkerInsertEXT(VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT *pMarkerInfo)
{
    struct vkCmdDebugMarkerInsertEXT_params params;
    params.commandBuffer = commandBuffer;
    params.pMarkerInfo = pMarkerInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdDebugMarkerInsertEXT, &params);
}

void WINAPI vkCmdDispatch(VkCommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    struct vkCmdDispatch_params params;
    params.commandBuffer = commandBuffer;
    params.groupCountX = groupCountX;
    params.groupCountY = groupCountY;
    params.groupCountZ = groupCountZ;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdDispatch, &params);
}

void WINAPI vkCmdDispatchBase(VkCommandBuffer commandBuffer, uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    struct vkCmdDispatchBase_params params;
    params.commandBuffer = commandBuffer;
    params.baseGroupX = baseGroupX;
    params.baseGroupY = baseGroupY;
    params.baseGroupZ = baseGroupZ;
    params.groupCountX = groupCountX;
    params.groupCountY = groupCountY;
    params.groupCountZ = groupCountZ;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdDispatchBase, &params);
}

void WINAPI vkCmdDispatchBaseKHR(VkCommandBuffer commandBuffer, uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    struct vkCmdDispatchBaseKHR_params params;
    params.commandBuffer = commandBuffer;
    params.baseGroupX = baseGroupX;
    params.baseGroupY = baseGroupY;
    params.baseGroupZ = baseGroupZ;
    params.groupCountX = groupCountX;
    params.groupCountY = groupCountY;
    params.groupCountZ = groupCountZ;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdDispatchBaseKHR, &params);
}

void WINAPI vkCmdDispatchIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset)
{
    struct vkCmdDispatchIndirect_params params;
    params.commandBuffer = commandBuffer;
    params.buffer = buffer;
    params.offset = offset;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdDispatchIndirect, &params);
}

void WINAPI vkCmdDraw(VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    struct vkCmdDraw_params params;
    params.commandBuffer = commandBuffer;
    params.vertexCount = vertexCount;
    params.instanceCount = instanceCount;
    params.firstVertex = firstVertex;
    params.firstInstance = firstInstance;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdDraw, &params);
}

void WINAPI vkCmdDrawIndexed(VkCommandBuffer commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
{
    struct vkCmdDrawIndexed_params params;
    params.commandBuffer = commandBuffer;
    params.indexCount = indexCount;
    params.instanceCount = instanceCount;
    params.firstIndex = firstIndex;
    params.vertexOffset = vertexOffset;
    params.firstInstance = firstInstance;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdDrawIndexed, &params);
}

void WINAPI vkCmdDrawIndexedIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
{
    struct vkCmdDrawIndexedIndirect_params params;
    params.commandBuffer = commandBuffer;
    params.buffer = buffer;
    params.offset = offset;
    params.drawCount = drawCount;
    params.stride = stride;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdDrawIndexedIndirect, &params);
}

void WINAPI vkCmdDrawIndexedIndirectCount(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
    struct vkCmdDrawIndexedIndirectCount_params params;
    params.commandBuffer = commandBuffer;
    params.buffer = buffer;
    params.offset = offset;
    params.countBuffer = countBuffer;
    params.countBufferOffset = countBufferOffset;
    params.maxDrawCount = maxDrawCount;
    params.stride = stride;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdDrawIndexedIndirectCount, &params);
}

void WINAPI vkCmdDrawIndexedIndirectCountAMD(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
    struct vkCmdDrawIndexedIndirectCountAMD_params params;
    params.commandBuffer = commandBuffer;
    params.buffer = buffer;
    params.offset = offset;
    params.countBuffer = countBuffer;
    params.countBufferOffset = countBufferOffset;
    params.maxDrawCount = maxDrawCount;
    params.stride = stride;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdDrawIndexedIndirectCountAMD, &params);
}

void WINAPI vkCmdDrawIndexedIndirectCountKHR(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
    struct vkCmdDrawIndexedIndirectCountKHR_params params;
    params.commandBuffer = commandBuffer;
    params.buffer = buffer;
    params.offset = offset;
    params.countBuffer = countBuffer;
    params.countBufferOffset = countBufferOffset;
    params.maxDrawCount = maxDrawCount;
    params.stride = stride;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdDrawIndexedIndirectCountKHR, &params);
}

void WINAPI vkCmdDrawIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
{
    struct vkCmdDrawIndirect_params params;
    params.commandBuffer = commandBuffer;
    params.buffer = buffer;
    params.offset = offset;
    params.drawCount = drawCount;
    params.stride = stride;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdDrawIndirect, &params);
}

void WINAPI vkCmdDrawIndirectByteCountEXT(VkCommandBuffer commandBuffer, uint32_t instanceCount, uint32_t firstInstance, VkBuffer counterBuffer, VkDeviceSize counterBufferOffset, uint32_t counterOffset, uint32_t vertexStride)
{
    struct vkCmdDrawIndirectByteCountEXT_params params;
    params.commandBuffer = commandBuffer;
    params.instanceCount = instanceCount;
    params.firstInstance = firstInstance;
    params.counterBuffer = counterBuffer;
    params.counterBufferOffset = counterBufferOffset;
    params.counterOffset = counterOffset;
    params.vertexStride = vertexStride;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdDrawIndirectByteCountEXT, &params);
}

void WINAPI vkCmdDrawIndirectCount(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
    struct vkCmdDrawIndirectCount_params params;
    params.commandBuffer = commandBuffer;
    params.buffer = buffer;
    params.offset = offset;
    params.countBuffer = countBuffer;
    params.countBufferOffset = countBufferOffset;
    params.maxDrawCount = maxDrawCount;
    params.stride = stride;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdDrawIndirectCount, &params);
}

void WINAPI vkCmdDrawIndirectCountAMD(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
    struct vkCmdDrawIndirectCountAMD_params params;
    params.commandBuffer = commandBuffer;
    params.buffer = buffer;
    params.offset = offset;
    params.countBuffer = countBuffer;
    params.countBufferOffset = countBufferOffset;
    params.maxDrawCount = maxDrawCount;
    params.stride = stride;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdDrawIndirectCountAMD, &params);
}

void WINAPI vkCmdDrawIndirectCountKHR(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
    struct vkCmdDrawIndirectCountKHR_params params;
    params.commandBuffer = commandBuffer;
    params.buffer = buffer;
    params.offset = offset;
    params.countBuffer = countBuffer;
    params.countBufferOffset = countBufferOffset;
    params.maxDrawCount = maxDrawCount;
    params.stride = stride;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdDrawIndirectCountKHR, &params);
}

void WINAPI vkCmdDrawMeshTasksEXT(VkCommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    struct vkCmdDrawMeshTasksEXT_params params;
    params.commandBuffer = commandBuffer;
    params.groupCountX = groupCountX;
    params.groupCountY = groupCountY;
    params.groupCountZ = groupCountZ;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdDrawMeshTasksEXT, &params);
}

void WINAPI vkCmdDrawMeshTasksIndirectCountEXT(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
    struct vkCmdDrawMeshTasksIndirectCountEXT_params params;
    params.commandBuffer = commandBuffer;
    params.buffer = buffer;
    params.offset = offset;
    params.countBuffer = countBuffer;
    params.countBufferOffset = countBufferOffset;
    params.maxDrawCount = maxDrawCount;
    params.stride = stride;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdDrawMeshTasksIndirectCountEXT, &params);
}

void WINAPI vkCmdDrawMeshTasksIndirectCountNV(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
    struct vkCmdDrawMeshTasksIndirectCountNV_params params;
    params.commandBuffer = commandBuffer;
    params.buffer = buffer;
    params.offset = offset;
    params.countBuffer = countBuffer;
    params.countBufferOffset = countBufferOffset;
    params.maxDrawCount = maxDrawCount;
    params.stride = stride;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdDrawMeshTasksIndirectCountNV, &params);
}

void WINAPI vkCmdDrawMeshTasksIndirectEXT(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
{
    struct vkCmdDrawMeshTasksIndirectEXT_params params;
    params.commandBuffer = commandBuffer;
    params.buffer = buffer;
    params.offset = offset;
    params.drawCount = drawCount;
    params.stride = stride;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdDrawMeshTasksIndirectEXT, &params);
}

void WINAPI vkCmdDrawMeshTasksIndirectNV(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
{
    struct vkCmdDrawMeshTasksIndirectNV_params params;
    params.commandBuffer = commandBuffer;
    params.buffer = buffer;
    params.offset = offset;
    params.drawCount = drawCount;
    params.stride = stride;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdDrawMeshTasksIndirectNV, &params);
}

void WINAPI vkCmdDrawMeshTasksNV(VkCommandBuffer commandBuffer, uint32_t taskCount, uint32_t firstTask)
{
    struct vkCmdDrawMeshTasksNV_params params;
    params.commandBuffer = commandBuffer;
    params.taskCount = taskCount;
    params.firstTask = firstTask;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdDrawMeshTasksNV, &params);
}

void WINAPI vkCmdDrawMultiEXT(VkCommandBuffer commandBuffer, uint32_t drawCount, const VkMultiDrawInfoEXT *pVertexInfo, uint32_t instanceCount, uint32_t firstInstance, uint32_t stride)
{
    struct vkCmdDrawMultiEXT_params params;
    params.commandBuffer = commandBuffer;
    params.drawCount = drawCount;
    params.pVertexInfo = pVertexInfo;
    params.instanceCount = instanceCount;
    params.firstInstance = firstInstance;
    params.stride = stride;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdDrawMultiEXT, &params);
}

void WINAPI vkCmdDrawMultiIndexedEXT(VkCommandBuffer commandBuffer, uint32_t drawCount, const VkMultiDrawIndexedInfoEXT *pIndexInfo, uint32_t instanceCount, uint32_t firstInstance, uint32_t stride, const int32_t *pVertexOffset)
{
    struct vkCmdDrawMultiIndexedEXT_params params;
    params.commandBuffer = commandBuffer;
    params.drawCount = drawCount;
    params.pIndexInfo = pIndexInfo;
    params.instanceCount = instanceCount;
    params.firstInstance = firstInstance;
    params.stride = stride;
    params.pVertexOffset = pVertexOffset;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdDrawMultiIndexedEXT, &params);
}

void WINAPI vkCmdEndConditionalRenderingEXT(VkCommandBuffer commandBuffer)
{
    struct vkCmdEndConditionalRenderingEXT_params params;
    params.commandBuffer = commandBuffer;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdEndConditionalRenderingEXT, &params);
}

void WINAPI vkCmdEndDebugUtilsLabelEXT(VkCommandBuffer commandBuffer)
{
    struct vkCmdEndDebugUtilsLabelEXT_params params;
    params.commandBuffer = commandBuffer;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdEndDebugUtilsLabelEXT, &params);
}

void WINAPI vkCmdEndQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query)
{
    struct vkCmdEndQuery_params params;
    params.commandBuffer = commandBuffer;
    params.queryPool = queryPool;
    params.query = query;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdEndQuery, &params);
}

void WINAPI vkCmdEndQueryIndexedEXT(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query, uint32_t index)
{
    struct vkCmdEndQueryIndexedEXT_params params;
    params.commandBuffer = commandBuffer;
    params.queryPool = queryPool;
    params.query = query;
    params.index = index;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdEndQueryIndexedEXT, &params);
}

void WINAPI vkCmdEndRenderPass(VkCommandBuffer commandBuffer)
{
    struct vkCmdEndRenderPass_params params;
    params.commandBuffer = commandBuffer;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdEndRenderPass, &params);
}

void WINAPI vkCmdEndRenderPass2(VkCommandBuffer commandBuffer, const VkSubpassEndInfo *pSubpassEndInfo)
{
    struct vkCmdEndRenderPass2_params params;
    params.commandBuffer = commandBuffer;
    params.pSubpassEndInfo = pSubpassEndInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdEndRenderPass2, &params);
}

void WINAPI vkCmdEndRenderPass2KHR(VkCommandBuffer commandBuffer, const VkSubpassEndInfo *pSubpassEndInfo)
{
    struct vkCmdEndRenderPass2KHR_params params;
    params.commandBuffer = commandBuffer;
    params.pSubpassEndInfo = pSubpassEndInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdEndRenderPass2KHR, &params);
}

void WINAPI vkCmdEndRendering(VkCommandBuffer commandBuffer)
{
    struct vkCmdEndRendering_params params;
    params.commandBuffer = commandBuffer;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdEndRendering, &params);
}

void WINAPI vkCmdEndRenderingKHR(VkCommandBuffer commandBuffer)
{
    struct vkCmdEndRenderingKHR_params params;
    params.commandBuffer = commandBuffer;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdEndRenderingKHR, &params);
}

void WINAPI vkCmdEndTransformFeedbackEXT(VkCommandBuffer commandBuffer, uint32_t firstCounterBuffer, uint32_t counterBufferCount, const VkBuffer *pCounterBuffers, const VkDeviceSize *pCounterBufferOffsets)
{
    struct vkCmdEndTransformFeedbackEXT_params params;
    params.commandBuffer = commandBuffer;
    params.firstCounterBuffer = firstCounterBuffer;
    params.counterBufferCount = counterBufferCount;
    params.pCounterBuffers = pCounterBuffers;
    params.pCounterBufferOffsets = pCounterBufferOffsets;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdEndTransformFeedbackEXT, &params);
}

void WINAPI vkCmdExecuteCommands(VkCommandBuffer commandBuffer, uint32_t commandBufferCount, const VkCommandBuffer *pCommandBuffers)
{
    struct vkCmdExecuteCommands_params params;
    params.commandBuffer = commandBuffer;
    params.commandBufferCount = commandBufferCount;
    params.pCommandBuffers = pCommandBuffers;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdExecuteCommands, &params);
}

void WINAPI vkCmdExecuteGeneratedCommandsNV(VkCommandBuffer commandBuffer, VkBool32 isPreprocessed, const VkGeneratedCommandsInfoNV *pGeneratedCommandsInfo)
{
    struct vkCmdExecuteGeneratedCommandsNV_params params;
    params.commandBuffer = commandBuffer;
    params.isPreprocessed = isPreprocessed;
    params.pGeneratedCommandsInfo = pGeneratedCommandsInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdExecuteGeneratedCommandsNV, &params);
}

void WINAPI vkCmdFillBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data)
{
    struct vkCmdFillBuffer_params params;
    params.commandBuffer = commandBuffer;
    params.dstBuffer = dstBuffer;
    params.dstOffset = dstOffset;
    params.size = size;
    params.data = data;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdFillBuffer, &params);
}

void WINAPI vkCmdInsertDebugUtilsLabelEXT(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT *pLabelInfo)
{
    struct vkCmdInsertDebugUtilsLabelEXT_params params;
    params.commandBuffer = commandBuffer;
    params.pLabelInfo = pLabelInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdInsertDebugUtilsLabelEXT, &params);
}

void WINAPI vkCmdNextSubpass(VkCommandBuffer commandBuffer, VkSubpassContents contents)
{
    struct vkCmdNextSubpass_params params;
    params.commandBuffer = commandBuffer;
    params.contents = contents;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdNextSubpass, &params);
}

void WINAPI vkCmdNextSubpass2(VkCommandBuffer commandBuffer, const VkSubpassBeginInfo *pSubpassBeginInfo, const VkSubpassEndInfo *pSubpassEndInfo)
{
    struct vkCmdNextSubpass2_params params;
    params.commandBuffer = commandBuffer;
    params.pSubpassBeginInfo = pSubpassBeginInfo;
    params.pSubpassEndInfo = pSubpassEndInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdNextSubpass2, &params);
}

void WINAPI vkCmdNextSubpass2KHR(VkCommandBuffer commandBuffer, const VkSubpassBeginInfo *pSubpassBeginInfo, const VkSubpassEndInfo *pSubpassEndInfo)
{
    struct vkCmdNextSubpass2KHR_params params;
    params.commandBuffer = commandBuffer;
    params.pSubpassBeginInfo = pSubpassBeginInfo;
    params.pSubpassEndInfo = pSubpassEndInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdNextSubpass2KHR, &params);
}

void WINAPI vkCmdOpticalFlowExecuteNV(VkCommandBuffer commandBuffer, VkOpticalFlowSessionNV session, const VkOpticalFlowExecuteInfoNV *pExecuteInfo)
{
    struct vkCmdOpticalFlowExecuteNV_params params;
    params.commandBuffer = commandBuffer;
    params.session = session;
    params.pExecuteInfo = pExecuteInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdOpticalFlowExecuteNV, &params);
}

void WINAPI vkCmdPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier *pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier *pImageMemoryBarriers)
{
    struct vkCmdPipelineBarrier_params params;
    params.commandBuffer = commandBuffer;
    params.srcStageMask = srcStageMask;
    params.dstStageMask = dstStageMask;
    params.dependencyFlags = dependencyFlags;
    params.memoryBarrierCount = memoryBarrierCount;
    params.pMemoryBarriers = pMemoryBarriers;
    params.bufferMemoryBarrierCount = bufferMemoryBarrierCount;
    params.pBufferMemoryBarriers = pBufferMemoryBarriers;
    params.imageMemoryBarrierCount = imageMemoryBarrierCount;
    params.pImageMemoryBarriers = pImageMemoryBarriers;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdPipelineBarrier, &params);
}

void WINAPI vkCmdPipelineBarrier2(VkCommandBuffer commandBuffer, const VkDependencyInfo *pDependencyInfo)
{
    struct vkCmdPipelineBarrier2_params params;
    params.commandBuffer = commandBuffer;
    params.pDependencyInfo = pDependencyInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdPipelineBarrier2, &params);
}

void WINAPI vkCmdPipelineBarrier2KHR(VkCommandBuffer commandBuffer, const VkDependencyInfo *pDependencyInfo)
{
    struct vkCmdPipelineBarrier2KHR_params params;
    params.commandBuffer = commandBuffer;
    params.pDependencyInfo = pDependencyInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdPipelineBarrier2KHR, &params);
}

void WINAPI vkCmdPreprocessGeneratedCommandsNV(VkCommandBuffer commandBuffer, const VkGeneratedCommandsInfoNV *pGeneratedCommandsInfo)
{
    struct vkCmdPreprocessGeneratedCommandsNV_params params;
    params.commandBuffer = commandBuffer;
    params.pGeneratedCommandsInfo = pGeneratedCommandsInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdPreprocessGeneratedCommandsNV, &params);
}

void WINAPI vkCmdPushConstants(VkCommandBuffer commandBuffer, VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void *pValues)
{
    struct vkCmdPushConstants_params params;
    params.commandBuffer = commandBuffer;
    params.layout = layout;
    params.stageFlags = stageFlags;
    params.offset = offset;
    params.size = size;
    params.pValues = pValues;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdPushConstants, &params);
}

void WINAPI vkCmdPushDescriptorSetKHR(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t set, uint32_t descriptorWriteCount, const VkWriteDescriptorSet *pDescriptorWrites)
{
    struct vkCmdPushDescriptorSetKHR_params params;
    params.commandBuffer = commandBuffer;
    params.pipelineBindPoint = pipelineBindPoint;
    params.layout = layout;
    params.set = set;
    params.descriptorWriteCount = descriptorWriteCount;
    params.pDescriptorWrites = pDescriptorWrites;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdPushDescriptorSetKHR, &params);
}

void WINAPI vkCmdPushDescriptorSetWithTemplateKHR(VkCommandBuffer commandBuffer, VkDescriptorUpdateTemplate descriptorUpdateTemplate, VkPipelineLayout layout, uint32_t set, const void *pData)
{
    struct vkCmdPushDescriptorSetWithTemplateKHR_params params;
    params.commandBuffer = commandBuffer;
    params.descriptorUpdateTemplate = descriptorUpdateTemplate;
    params.layout = layout;
    params.set = set;
    params.pData = pData;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdPushDescriptorSetWithTemplateKHR, &params);
}

void WINAPI vkCmdResetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask)
{
    struct vkCmdResetEvent_params params;
    params.commandBuffer = commandBuffer;
    params.event = event;
    params.stageMask = stageMask;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdResetEvent, &params);
}

void WINAPI vkCmdResetEvent2(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags2 stageMask)
{
    struct vkCmdResetEvent2_params params;
    params.commandBuffer = commandBuffer;
    params.event = event;
    params.stageMask = stageMask;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdResetEvent2, &params);
}

void WINAPI vkCmdResetEvent2KHR(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags2 stageMask)
{
    struct vkCmdResetEvent2KHR_params params;
    params.commandBuffer = commandBuffer;
    params.event = event;
    params.stageMask = stageMask;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdResetEvent2KHR, &params);
}

void WINAPI vkCmdResetQueryPool(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount)
{
    struct vkCmdResetQueryPool_params params;
    params.commandBuffer = commandBuffer;
    params.queryPool = queryPool;
    params.firstQuery = firstQuery;
    params.queryCount = queryCount;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdResetQueryPool, &params);
}

void WINAPI vkCmdResolveImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageResolve *pRegions)
{
    struct vkCmdResolveImage_params params;
    params.commandBuffer = commandBuffer;
    params.srcImage = srcImage;
    params.srcImageLayout = srcImageLayout;
    params.dstImage = dstImage;
    params.dstImageLayout = dstImageLayout;
    params.regionCount = regionCount;
    params.pRegions = pRegions;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdResolveImage, &params);
}

void WINAPI vkCmdResolveImage2(VkCommandBuffer commandBuffer, const VkResolveImageInfo2 *pResolveImageInfo)
{
    struct vkCmdResolveImage2_params params;
    params.commandBuffer = commandBuffer;
    params.pResolveImageInfo = pResolveImageInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdResolveImage2, &params);
}

void WINAPI vkCmdResolveImage2KHR(VkCommandBuffer commandBuffer, const VkResolveImageInfo2 *pResolveImageInfo)
{
    struct vkCmdResolveImage2KHR_params params;
    params.commandBuffer = commandBuffer;
    params.pResolveImageInfo = pResolveImageInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdResolveImage2KHR, &params);
}

void WINAPI vkCmdSetAlphaToCoverageEnableEXT(VkCommandBuffer commandBuffer, VkBool32 alphaToCoverageEnable)
{
    struct vkCmdSetAlphaToCoverageEnableEXT_params params;
    params.commandBuffer = commandBuffer;
    params.alphaToCoverageEnable = alphaToCoverageEnable;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetAlphaToCoverageEnableEXT, &params);
}

void WINAPI vkCmdSetAlphaToOneEnableEXT(VkCommandBuffer commandBuffer, VkBool32 alphaToOneEnable)
{
    struct vkCmdSetAlphaToOneEnableEXT_params params;
    params.commandBuffer = commandBuffer;
    params.alphaToOneEnable = alphaToOneEnable;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetAlphaToOneEnableEXT, &params);
}

void WINAPI vkCmdSetBlendConstants(VkCommandBuffer commandBuffer, const float blendConstants[4])
{
    struct vkCmdSetBlendConstants_params params;
    params.commandBuffer = commandBuffer;
    params.blendConstants = blendConstants;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetBlendConstants, &params);
}

void WINAPI vkCmdSetCheckpointNV(VkCommandBuffer commandBuffer, const void *pCheckpointMarker)
{
    struct vkCmdSetCheckpointNV_params params;
    params.commandBuffer = commandBuffer;
    params.pCheckpointMarker = pCheckpointMarker;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetCheckpointNV, &params);
}

void WINAPI vkCmdSetCoarseSampleOrderNV(VkCommandBuffer commandBuffer, VkCoarseSampleOrderTypeNV sampleOrderType, uint32_t customSampleOrderCount, const VkCoarseSampleOrderCustomNV *pCustomSampleOrders)
{
    struct vkCmdSetCoarseSampleOrderNV_params params;
    params.commandBuffer = commandBuffer;
    params.sampleOrderType = sampleOrderType;
    params.customSampleOrderCount = customSampleOrderCount;
    params.pCustomSampleOrders = pCustomSampleOrders;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetCoarseSampleOrderNV, &params);
}

void WINAPI vkCmdSetColorBlendAdvancedEXT(VkCommandBuffer commandBuffer, uint32_t firstAttachment, uint32_t attachmentCount, const VkColorBlendAdvancedEXT *pColorBlendAdvanced)
{
    struct vkCmdSetColorBlendAdvancedEXT_params params;
    params.commandBuffer = commandBuffer;
    params.firstAttachment = firstAttachment;
    params.attachmentCount = attachmentCount;
    params.pColorBlendAdvanced = pColorBlendAdvanced;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetColorBlendAdvancedEXT, &params);
}

void WINAPI vkCmdSetColorBlendEnableEXT(VkCommandBuffer commandBuffer, uint32_t firstAttachment, uint32_t attachmentCount, const VkBool32 *pColorBlendEnables)
{
    struct vkCmdSetColorBlendEnableEXT_params params;
    params.commandBuffer = commandBuffer;
    params.firstAttachment = firstAttachment;
    params.attachmentCount = attachmentCount;
    params.pColorBlendEnables = pColorBlendEnables;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetColorBlendEnableEXT, &params);
}

void WINAPI vkCmdSetColorBlendEquationEXT(VkCommandBuffer commandBuffer, uint32_t firstAttachment, uint32_t attachmentCount, const VkColorBlendEquationEXT *pColorBlendEquations)
{
    struct vkCmdSetColorBlendEquationEXT_params params;
    params.commandBuffer = commandBuffer;
    params.firstAttachment = firstAttachment;
    params.attachmentCount = attachmentCount;
    params.pColorBlendEquations = pColorBlendEquations;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetColorBlendEquationEXT, &params);
}

void WINAPI vkCmdSetColorWriteEnableEXT(VkCommandBuffer commandBuffer, uint32_t attachmentCount, const VkBool32 *pColorWriteEnables)
{
    struct vkCmdSetColorWriteEnableEXT_params params;
    params.commandBuffer = commandBuffer;
    params.attachmentCount = attachmentCount;
    params.pColorWriteEnables = pColorWriteEnables;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetColorWriteEnableEXT, &params);
}

void WINAPI vkCmdSetColorWriteMaskEXT(VkCommandBuffer commandBuffer, uint32_t firstAttachment, uint32_t attachmentCount, const VkColorComponentFlags *pColorWriteMasks)
{
    struct vkCmdSetColorWriteMaskEXT_params params;
    params.commandBuffer = commandBuffer;
    params.firstAttachment = firstAttachment;
    params.attachmentCount = attachmentCount;
    params.pColorWriteMasks = pColorWriteMasks;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetColorWriteMaskEXT, &params);
}

void WINAPI vkCmdSetConservativeRasterizationModeEXT(VkCommandBuffer commandBuffer, VkConservativeRasterizationModeEXT conservativeRasterizationMode)
{
    struct vkCmdSetConservativeRasterizationModeEXT_params params;
    params.commandBuffer = commandBuffer;
    params.conservativeRasterizationMode = conservativeRasterizationMode;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetConservativeRasterizationModeEXT, &params);
}

void WINAPI vkCmdSetCoverageModulationModeNV(VkCommandBuffer commandBuffer, VkCoverageModulationModeNV coverageModulationMode)
{
    struct vkCmdSetCoverageModulationModeNV_params params;
    params.commandBuffer = commandBuffer;
    params.coverageModulationMode = coverageModulationMode;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetCoverageModulationModeNV, &params);
}

void WINAPI vkCmdSetCoverageModulationTableEnableNV(VkCommandBuffer commandBuffer, VkBool32 coverageModulationTableEnable)
{
    struct vkCmdSetCoverageModulationTableEnableNV_params params;
    params.commandBuffer = commandBuffer;
    params.coverageModulationTableEnable = coverageModulationTableEnable;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetCoverageModulationTableEnableNV, &params);
}

void WINAPI vkCmdSetCoverageModulationTableNV(VkCommandBuffer commandBuffer, uint32_t coverageModulationTableCount, const float *pCoverageModulationTable)
{
    struct vkCmdSetCoverageModulationTableNV_params params;
    params.commandBuffer = commandBuffer;
    params.coverageModulationTableCount = coverageModulationTableCount;
    params.pCoverageModulationTable = pCoverageModulationTable;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetCoverageModulationTableNV, &params);
}

void WINAPI vkCmdSetCoverageReductionModeNV(VkCommandBuffer commandBuffer, VkCoverageReductionModeNV coverageReductionMode)
{
    struct vkCmdSetCoverageReductionModeNV_params params;
    params.commandBuffer = commandBuffer;
    params.coverageReductionMode = coverageReductionMode;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetCoverageReductionModeNV, &params);
}

void WINAPI vkCmdSetCoverageToColorEnableNV(VkCommandBuffer commandBuffer, VkBool32 coverageToColorEnable)
{
    struct vkCmdSetCoverageToColorEnableNV_params params;
    params.commandBuffer = commandBuffer;
    params.coverageToColorEnable = coverageToColorEnable;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetCoverageToColorEnableNV, &params);
}

void WINAPI vkCmdSetCoverageToColorLocationNV(VkCommandBuffer commandBuffer, uint32_t coverageToColorLocation)
{
    struct vkCmdSetCoverageToColorLocationNV_params params;
    params.commandBuffer = commandBuffer;
    params.coverageToColorLocation = coverageToColorLocation;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetCoverageToColorLocationNV, &params);
}

void WINAPI vkCmdSetCullMode(VkCommandBuffer commandBuffer, VkCullModeFlags cullMode)
{
    struct vkCmdSetCullMode_params params;
    params.commandBuffer = commandBuffer;
    params.cullMode = cullMode;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetCullMode, &params);
}

void WINAPI vkCmdSetCullModeEXT(VkCommandBuffer commandBuffer, VkCullModeFlags cullMode)
{
    struct vkCmdSetCullModeEXT_params params;
    params.commandBuffer = commandBuffer;
    params.cullMode = cullMode;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetCullModeEXT, &params);
}

void WINAPI vkCmdSetDepthBias(VkCommandBuffer commandBuffer, float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor)
{
    struct vkCmdSetDepthBias_params params;
    params.commandBuffer = commandBuffer;
    params.depthBiasConstantFactor = depthBiasConstantFactor;
    params.depthBiasClamp = depthBiasClamp;
    params.depthBiasSlopeFactor = depthBiasSlopeFactor;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetDepthBias, &params);
}

void WINAPI vkCmdSetDepthBiasEnable(VkCommandBuffer commandBuffer, VkBool32 depthBiasEnable)
{
    struct vkCmdSetDepthBiasEnable_params params;
    params.commandBuffer = commandBuffer;
    params.depthBiasEnable = depthBiasEnable;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetDepthBiasEnable, &params);
}

void WINAPI vkCmdSetDepthBiasEnableEXT(VkCommandBuffer commandBuffer, VkBool32 depthBiasEnable)
{
    struct vkCmdSetDepthBiasEnableEXT_params params;
    params.commandBuffer = commandBuffer;
    params.depthBiasEnable = depthBiasEnable;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetDepthBiasEnableEXT, &params);
}

void WINAPI vkCmdSetDepthBounds(VkCommandBuffer commandBuffer, float minDepthBounds, float maxDepthBounds)
{
    struct vkCmdSetDepthBounds_params params;
    params.commandBuffer = commandBuffer;
    params.minDepthBounds = minDepthBounds;
    params.maxDepthBounds = maxDepthBounds;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetDepthBounds, &params);
}

void WINAPI vkCmdSetDepthBoundsTestEnable(VkCommandBuffer commandBuffer, VkBool32 depthBoundsTestEnable)
{
    struct vkCmdSetDepthBoundsTestEnable_params params;
    params.commandBuffer = commandBuffer;
    params.depthBoundsTestEnable = depthBoundsTestEnable;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetDepthBoundsTestEnable, &params);
}

void WINAPI vkCmdSetDepthBoundsTestEnableEXT(VkCommandBuffer commandBuffer, VkBool32 depthBoundsTestEnable)
{
    struct vkCmdSetDepthBoundsTestEnableEXT_params params;
    params.commandBuffer = commandBuffer;
    params.depthBoundsTestEnable = depthBoundsTestEnable;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetDepthBoundsTestEnableEXT, &params);
}

void WINAPI vkCmdSetDepthClampEnableEXT(VkCommandBuffer commandBuffer, VkBool32 depthClampEnable)
{
    struct vkCmdSetDepthClampEnableEXT_params params;
    params.commandBuffer = commandBuffer;
    params.depthClampEnable = depthClampEnable;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetDepthClampEnableEXT, &params);
}

void WINAPI vkCmdSetDepthClipEnableEXT(VkCommandBuffer commandBuffer, VkBool32 depthClipEnable)
{
    struct vkCmdSetDepthClipEnableEXT_params params;
    params.commandBuffer = commandBuffer;
    params.depthClipEnable = depthClipEnable;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetDepthClipEnableEXT, &params);
}

void WINAPI vkCmdSetDepthClipNegativeOneToOneEXT(VkCommandBuffer commandBuffer, VkBool32 negativeOneToOne)
{
    struct vkCmdSetDepthClipNegativeOneToOneEXT_params params;
    params.commandBuffer = commandBuffer;
    params.negativeOneToOne = negativeOneToOne;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetDepthClipNegativeOneToOneEXT, &params);
}

void WINAPI vkCmdSetDepthCompareOp(VkCommandBuffer commandBuffer, VkCompareOp depthCompareOp)
{
    struct vkCmdSetDepthCompareOp_params params;
    params.commandBuffer = commandBuffer;
    params.depthCompareOp = depthCompareOp;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetDepthCompareOp, &params);
}

void WINAPI vkCmdSetDepthCompareOpEXT(VkCommandBuffer commandBuffer, VkCompareOp depthCompareOp)
{
    struct vkCmdSetDepthCompareOpEXT_params params;
    params.commandBuffer = commandBuffer;
    params.depthCompareOp = depthCompareOp;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetDepthCompareOpEXT, &params);
}

void WINAPI vkCmdSetDepthTestEnable(VkCommandBuffer commandBuffer, VkBool32 depthTestEnable)
{
    struct vkCmdSetDepthTestEnable_params params;
    params.commandBuffer = commandBuffer;
    params.depthTestEnable = depthTestEnable;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetDepthTestEnable, &params);
}

void WINAPI vkCmdSetDepthTestEnableEXT(VkCommandBuffer commandBuffer, VkBool32 depthTestEnable)
{
    struct vkCmdSetDepthTestEnableEXT_params params;
    params.commandBuffer = commandBuffer;
    params.depthTestEnable = depthTestEnable;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetDepthTestEnableEXT, &params);
}

void WINAPI vkCmdSetDepthWriteEnable(VkCommandBuffer commandBuffer, VkBool32 depthWriteEnable)
{
    struct vkCmdSetDepthWriteEnable_params params;
    params.commandBuffer = commandBuffer;
    params.depthWriteEnable = depthWriteEnable;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetDepthWriteEnable, &params);
}

void WINAPI vkCmdSetDepthWriteEnableEXT(VkCommandBuffer commandBuffer, VkBool32 depthWriteEnable)
{
    struct vkCmdSetDepthWriteEnableEXT_params params;
    params.commandBuffer = commandBuffer;
    params.depthWriteEnable = depthWriteEnable;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetDepthWriteEnableEXT, &params);
}

void WINAPI vkCmdSetDeviceMask(VkCommandBuffer commandBuffer, uint32_t deviceMask)
{
    struct vkCmdSetDeviceMask_params params;
    params.commandBuffer = commandBuffer;
    params.deviceMask = deviceMask;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetDeviceMask, &params);
}

void WINAPI vkCmdSetDeviceMaskKHR(VkCommandBuffer commandBuffer, uint32_t deviceMask)
{
    struct vkCmdSetDeviceMaskKHR_params params;
    params.commandBuffer = commandBuffer;
    params.deviceMask = deviceMask;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetDeviceMaskKHR, &params);
}

void WINAPI vkCmdSetDiscardRectangleEXT(VkCommandBuffer commandBuffer, uint32_t firstDiscardRectangle, uint32_t discardRectangleCount, const VkRect2D *pDiscardRectangles)
{
    struct vkCmdSetDiscardRectangleEXT_params params;
    params.commandBuffer = commandBuffer;
    params.firstDiscardRectangle = firstDiscardRectangle;
    params.discardRectangleCount = discardRectangleCount;
    params.pDiscardRectangles = pDiscardRectangles;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetDiscardRectangleEXT, &params);
}

void WINAPI vkCmdSetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask)
{
    struct vkCmdSetEvent_params params;
    params.commandBuffer = commandBuffer;
    params.event = event;
    params.stageMask = stageMask;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetEvent, &params);
}

void WINAPI vkCmdSetEvent2(VkCommandBuffer commandBuffer, VkEvent event, const VkDependencyInfo *pDependencyInfo)
{
    struct vkCmdSetEvent2_params params;
    params.commandBuffer = commandBuffer;
    params.event = event;
    params.pDependencyInfo = pDependencyInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetEvent2, &params);
}

void WINAPI vkCmdSetEvent2KHR(VkCommandBuffer commandBuffer, VkEvent event, const VkDependencyInfo *pDependencyInfo)
{
    struct vkCmdSetEvent2KHR_params params;
    params.commandBuffer = commandBuffer;
    params.event = event;
    params.pDependencyInfo = pDependencyInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetEvent2KHR, &params);
}

void WINAPI vkCmdSetExclusiveScissorNV(VkCommandBuffer commandBuffer, uint32_t firstExclusiveScissor, uint32_t exclusiveScissorCount, const VkRect2D *pExclusiveScissors)
{
    struct vkCmdSetExclusiveScissorNV_params params;
    params.commandBuffer = commandBuffer;
    params.firstExclusiveScissor = firstExclusiveScissor;
    params.exclusiveScissorCount = exclusiveScissorCount;
    params.pExclusiveScissors = pExclusiveScissors;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetExclusiveScissorNV, &params);
}

void WINAPI vkCmdSetExtraPrimitiveOverestimationSizeEXT(VkCommandBuffer commandBuffer, float extraPrimitiveOverestimationSize)
{
    struct vkCmdSetExtraPrimitiveOverestimationSizeEXT_params params;
    params.commandBuffer = commandBuffer;
    params.extraPrimitiveOverestimationSize = extraPrimitiveOverestimationSize;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetExtraPrimitiveOverestimationSizeEXT, &params);
}

void WINAPI vkCmdSetFragmentShadingRateEnumNV(VkCommandBuffer commandBuffer, VkFragmentShadingRateNV shadingRate, const VkFragmentShadingRateCombinerOpKHR combinerOps[2])
{
    struct vkCmdSetFragmentShadingRateEnumNV_params params;
    params.commandBuffer = commandBuffer;
    params.shadingRate = shadingRate;
    params.combinerOps = combinerOps;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetFragmentShadingRateEnumNV, &params);
}

void WINAPI vkCmdSetFragmentShadingRateKHR(VkCommandBuffer commandBuffer, const VkExtent2D *pFragmentSize, const VkFragmentShadingRateCombinerOpKHR combinerOps[2])
{
    struct vkCmdSetFragmentShadingRateKHR_params params;
    params.commandBuffer = commandBuffer;
    params.pFragmentSize = pFragmentSize;
    params.combinerOps = combinerOps;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetFragmentShadingRateKHR, &params);
}

void WINAPI vkCmdSetFrontFace(VkCommandBuffer commandBuffer, VkFrontFace frontFace)
{
    struct vkCmdSetFrontFace_params params;
    params.commandBuffer = commandBuffer;
    params.frontFace = frontFace;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetFrontFace, &params);
}

void WINAPI vkCmdSetFrontFaceEXT(VkCommandBuffer commandBuffer, VkFrontFace frontFace)
{
    struct vkCmdSetFrontFaceEXT_params params;
    params.commandBuffer = commandBuffer;
    params.frontFace = frontFace;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetFrontFaceEXT, &params);
}

void WINAPI vkCmdSetLineRasterizationModeEXT(VkCommandBuffer commandBuffer, VkLineRasterizationModeEXT lineRasterizationMode)
{
    struct vkCmdSetLineRasterizationModeEXT_params params;
    params.commandBuffer = commandBuffer;
    params.lineRasterizationMode = lineRasterizationMode;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetLineRasterizationModeEXT, &params);
}

void WINAPI vkCmdSetLineStippleEXT(VkCommandBuffer commandBuffer, uint32_t lineStippleFactor, uint16_t lineStipplePattern)
{
    struct vkCmdSetLineStippleEXT_params params;
    params.commandBuffer = commandBuffer;
    params.lineStippleFactor = lineStippleFactor;
    params.lineStipplePattern = lineStipplePattern;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetLineStippleEXT, &params);
}

void WINAPI vkCmdSetLineStippleEnableEXT(VkCommandBuffer commandBuffer, VkBool32 stippledLineEnable)
{
    struct vkCmdSetLineStippleEnableEXT_params params;
    params.commandBuffer = commandBuffer;
    params.stippledLineEnable = stippledLineEnable;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetLineStippleEnableEXT, &params);
}

void WINAPI vkCmdSetLineWidth(VkCommandBuffer commandBuffer, float lineWidth)
{
    struct vkCmdSetLineWidth_params params;
    params.commandBuffer = commandBuffer;
    params.lineWidth = lineWidth;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetLineWidth, &params);
}

void WINAPI vkCmdSetLogicOpEXT(VkCommandBuffer commandBuffer, VkLogicOp logicOp)
{
    struct vkCmdSetLogicOpEXT_params params;
    params.commandBuffer = commandBuffer;
    params.logicOp = logicOp;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetLogicOpEXT, &params);
}

void WINAPI vkCmdSetLogicOpEnableEXT(VkCommandBuffer commandBuffer, VkBool32 logicOpEnable)
{
    struct vkCmdSetLogicOpEnableEXT_params params;
    params.commandBuffer = commandBuffer;
    params.logicOpEnable = logicOpEnable;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetLogicOpEnableEXT, &params);
}

void WINAPI vkCmdSetPatchControlPointsEXT(VkCommandBuffer commandBuffer, uint32_t patchControlPoints)
{
    struct vkCmdSetPatchControlPointsEXT_params params;
    params.commandBuffer = commandBuffer;
    params.patchControlPoints = patchControlPoints;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetPatchControlPointsEXT, &params);
}

VkResult WINAPI vkCmdSetPerformanceMarkerINTEL(VkCommandBuffer commandBuffer, const VkPerformanceMarkerInfoINTEL *pMarkerInfo)
{
    struct vkCmdSetPerformanceMarkerINTEL_params params;
    params.commandBuffer = commandBuffer;
    params.pMarkerInfo = pMarkerInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetPerformanceMarkerINTEL, &params);
    return params.result;
}

VkResult WINAPI vkCmdSetPerformanceOverrideINTEL(VkCommandBuffer commandBuffer, const VkPerformanceOverrideInfoINTEL *pOverrideInfo)
{
    struct vkCmdSetPerformanceOverrideINTEL_params params;
    params.commandBuffer = commandBuffer;
    params.pOverrideInfo = pOverrideInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetPerformanceOverrideINTEL, &params);
    return params.result;
}

VkResult WINAPI vkCmdSetPerformanceStreamMarkerINTEL(VkCommandBuffer commandBuffer, const VkPerformanceStreamMarkerInfoINTEL *pMarkerInfo)
{
    struct vkCmdSetPerformanceStreamMarkerINTEL_params params;
    params.commandBuffer = commandBuffer;
    params.pMarkerInfo = pMarkerInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetPerformanceStreamMarkerINTEL, &params);
    return params.result;
}

void WINAPI vkCmdSetPolygonModeEXT(VkCommandBuffer commandBuffer, VkPolygonMode polygonMode)
{
    struct vkCmdSetPolygonModeEXT_params params;
    params.commandBuffer = commandBuffer;
    params.polygonMode = polygonMode;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetPolygonModeEXT, &params);
}

void WINAPI vkCmdSetPrimitiveRestartEnable(VkCommandBuffer commandBuffer, VkBool32 primitiveRestartEnable)
{
    struct vkCmdSetPrimitiveRestartEnable_params params;
    params.commandBuffer = commandBuffer;
    params.primitiveRestartEnable = primitiveRestartEnable;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetPrimitiveRestartEnable, &params);
}

void WINAPI vkCmdSetPrimitiveRestartEnableEXT(VkCommandBuffer commandBuffer, VkBool32 primitiveRestartEnable)
{
    struct vkCmdSetPrimitiveRestartEnableEXT_params params;
    params.commandBuffer = commandBuffer;
    params.primitiveRestartEnable = primitiveRestartEnable;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetPrimitiveRestartEnableEXT, &params);
}

void WINAPI vkCmdSetPrimitiveTopology(VkCommandBuffer commandBuffer, VkPrimitiveTopology primitiveTopology)
{
    struct vkCmdSetPrimitiveTopology_params params;
    params.commandBuffer = commandBuffer;
    params.primitiveTopology = primitiveTopology;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetPrimitiveTopology, &params);
}

void WINAPI vkCmdSetPrimitiveTopologyEXT(VkCommandBuffer commandBuffer, VkPrimitiveTopology primitiveTopology)
{
    struct vkCmdSetPrimitiveTopologyEXT_params params;
    params.commandBuffer = commandBuffer;
    params.primitiveTopology = primitiveTopology;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetPrimitiveTopologyEXT, &params);
}

void WINAPI vkCmdSetProvokingVertexModeEXT(VkCommandBuffer commandBuffer, VkProvokingVertexModeEXT provokingVertexMode)
{
    struct vkCmdSetProvokingVertexModeEXT_params params;
    params.commandBuffer = commandBuffer;
    params.provokingVertexMode = provokingVertexMode;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetProvokingVertexModeEXT, &params);
}

void WINAPI vkCmdSetRasterizationSamplesEXT(VkCommandBuffer commandBuffer, VkSampleCountFlagBits rasterizationSamples)
{
    struct vkCmdSetRasterizationSamplesEXT_params params;
    params.commandBuffer = commandBuffer;
    params.rasterizationSamples = rasterizationSamples;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetRasterizationSamplesEXT, &params);
}

void WINAPI vkCmdSetRasterizationStreamEXT(VkCommandBuffer commandBuffer, uint32_t rasterizationStream)
{
    struct vkCmdSetRasterizationStreamEXT_params params;
    params.commandBuffer = commandBuffer;
    params.rasterizationStream = rasterizationStream;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetRasterizationStreamEXT, &params);
}

void WINAPI vkCmdSetRasterizerDiscardEnable(VkCommandBuffer commandBuffer, VkBool32 rasterizerDiscardEnable)
{
    struct vkCmdSetRasterizerDiscardEnable_params params;
    params.commandBuffer = commandBuffer;
    params.rasterizerDiscardEnable = rasterizerDiscardEnable;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetRasterizerDiscardEnable, &params);
}

void WINAPI vkCmdSetRasterizerDiscardEnableEXT(VkCommandBuffer commandBuffer, VkBool32 rasterizerDiscardEnable)
{
    struct vkCmdSetRasterizerDiscardEnableEXT_params params;
    params.commandBuffer = commandBuffer;
    params.rasterizerDiscardEnable = rasterizerDiscardEnable;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetRasterizerDiscardEnableEXT, &params);
}

void WINAPI vkCmdSetRayTracingPipelineStackSizeKHR(VkCommandBuffer commandBuffer, uint32_t pipelineStackSize)
{
    struct vkCmdSetRayTracingPipelineStackSizeKHR_params params;
    params.commandBuffer = commandBuffer;
    params.pipelineStackSize = pipelineStackSize;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetRayTracingPipelineStackSizeKHR, &params);
}

void WINAPI vkCmdSetRepresentativeFragmentTestEnableNV(VkCommandBuffer commandBuffer, VkBool32 representativeFragmentTestEnable)
{
    struct vkCmdSetRepresentativeFragmentTestEnableNV_params params;
    params.commandBuffer = commandBuffer;
    params.representativeFragmentTestEnable = representativeFragmentTestEnable;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetRepresentativeFragmentTestEnableNV, &params);
}

void WINAPI vkCmdSetSampleLocationsEXT(VkCommandBuffer commandBuffer, const VkSampleLocationsInfoEXT *pSampleLocationsInfo)
{
    struct vkCmdSetSampleLocationsEXT_params params;
    params.commandBuffer = commandBuffer;
    params.pSampleLocationsInfo = pSampleLocationsInfo;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetSampleLocationsEXT, &params);
}

void WINAPI vkCmdSetSampleLocationsEnableEXT(VkCommandBuffer commandBuffer, VkBool32 sampleLocationsEnable)
{
    struct vkCmdSetSampleLocationsEnableEXT_params params;
    params.commandBuffer = commandBuffer;
    params.sampleLocationsEnable = sampleLocationsEnable;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetSampleLocationsEnableEXT, &params);
}

void WINAPI vkCmdSetSampleMaskEXT(VkCommandBuffer commandBuffer, VkSampleCountFlagBits samples, const VkSampleMask *pSampleMask)
{
    struct vkCmdSetSampleMaskEXT_params params;
    params.commandBuffer = commandBuffer;
    params.samples = samples;
    params.pSampleMask = pSampleMask;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetSampleMaskEXT, &params);
}

void WINAPI vkCmdSetScissor(VkCommandBuffer commandBuffer, uint32_t firstScissor, uint32_t scissorCount, const VkRect2D *pScissors)
{
    struct vkCmdSetScissor_params params;
    params.commandBuffer = commandBuffer;
    params.firstScissor = firstScissor;
    params.scissorCount = scissorCount;
    params.pScissors = pScissors;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetScissor, &params);
}

void WINAPI vkCmdSetScissorWithCount(VkCommandBuffer commandBuffer, uint32_t scissorCount, const VkRect2D *pScissors)
{
    struct vkCmdSetScissorWithCount_params params;
    params.commandBuffer = commandBuffer;
    params.scissorCount = scissorCount;
    params.pScissors = pScissors;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetScissorWithCount, &params);
}

void WINAPI vkCmdSetScissorWithCountEXT(VkCommandBuffer commandBuffer, uint32_t scissorCount, const VkRect2D *pScissors)
{
    struct vkCmdSetScissorWithCountEXT_params params;
    params.commandBuffer = commandBuffer;
    params.scissorCount = scissorCount;
    params.pScissors = pScissors;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetScissorWithCountEXT, &params);
}

void WINAPI vkCmdSetShadingRateImageEnableNV(VkCommandBuffer commandBuffer, VkBool32 shadingRateImageEnable)
{
    struct vkCmdSetShadingRateImageEnableNV_params params;
    params.commandBuffer = commandBuffer;
    params.shadingRateImageEnable = shadingRateImageEnable;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetShadingRateImageEnableNV, &params);
}

void WINAPI vkCmdSetStencilCompareMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t compareMask)
{
    struct vkCmdSetStencilCompareMask_params params;
    params.commandBuffer = commandBuffer;
    params.faceMask = faceMask;
    params.compareMask = compareMask;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetStencilCompareMask, &params);
}

void WINAPI vkCmdSetStencilOp(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, VkStencilOp failOp, VkStencilOp passOp, VkStencilOp depthFailOp, VkCompareOp compareOp)
{
    struct vkCmdSetStencilOp_params params;
    params.commandBuffer = commandBuffer;
    params.faceMask = faceMask;
    params.failOp = failOp;
    params.passOp = passOp;
    params.depthFailOp = depthFailOp;
    params.compareOp = compareOp;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetStencilOp, &params);
}

void WINAPI vkCmdSetStencilOpEXT(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, VkStencilOp failOp, VkStencilOp passOp, VkStencilOp depthFailOp, VkCompareOp compareOp)
{
    struct vkCmdSetStencilOpEXT_params params;
    params.commandBuffer = commandBuffer;
    params.faceMask = faceMask;
    params.failOp = failOp;
    params.passOp = passOp;
    params.depthFailOp = depthFailOp;
    params.compareOp = compareOp;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetStencilOpEXT, &params);
}

void WINAPI vkCmdSetStencilReference(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t reference)
{
    struct vkCmdSetStencilReference_params params;
    params.commandBuffer = commandBuffer;
    params.faceMask = faceMask;
    params.reference = reference;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetStencilReference, &params);
}

void WINAPI vkCmdSetStencilTestEnable(VkCommandBuffer commandBuffer, VkBool32 stencilTestEnable)
{
    struct vkCmdSetStencilTestEnable_params params;
    params.commandBuffer = commandBuffer;
    params.stencilTestEnable = stencilTestEnable;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetStencilTestEnable, &params);
}

void WINAPI vkCmdSetStencilTestEnableEXT(VkCommandBuffer commandBuffer, VkBool32 stencilTestEnable)
{
    struct vkCmdSetStencilTestEnableEXT_params params;
    params.commandBuffer = commandBuffer;
    params.stencilTestEnable = stencilTestEnable;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetStencilTestEnableEXT, &params);
}

void WINAPI vkCmdSetStencilWriteMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t writeMask)
{
    struct vkCmdSetStencilWriteMask_params params;
    params.commandBuffer = commandBuffer;
    params.faceMask = faceMask;
    params.writeMask = writeMask;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetStencilWriteMask, &params);
}

void WINAPI vkCmdSetTessellationDomainOriginEXT(VkCommandBuffer commandBuffer, VkTessellationDomainOrigin domainOrigin)
{
    struct vkCmdSetTessellationDomainOriginEXT_params params;
    params.commandBuffer = commandBuffer;
    params.domainOrigin = domainOrigin;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetTessellationDomainOriginEXT, &params);
}

void WINAPI vkCmdSetVertexInputEXT(VkCommandBuffer commandBuffer, uint32_t vertexBindingDescriptionCount, const VkVertexInputBindingDescription2EXT *pVertexBindingDescriptions, uint32_t vertexAttributeDescriptionCount, const VkVertexInputAttributeDescription2EXT *pVertexAttributeDescriptions)
{
    struct vkCmdSetVertexInputEXT_params params;
    params.commandBuffer = commandBuffer;
    params.vertexBindingDescriptionCount = vertexBindingDescriptionCount;
    params.pVertexBindingDescriptions = pVertexBindingDescriptions;
    params.vertexAttributeDescriptionCount = vertexAttributeDescriptionCount;
    params.pVertexAttributeDescriptions = pVertexAttributeDescriptions;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetVertexInputEXT, &params);
}

void WINAPI vkCmdSetViewport(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkViewport *pViewports)
{
    struct vkCmdSetViewport_params params;
    params.commandBuffer = commandBuffer;
    params.firstViewport = firstViewport;
    params.viewportCount = viewportCount;
    params.pViewports = pViewports;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetViewport, &params);
}

void WINAPI vkCmdSetViewportShadingRatePaletteNV(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkShadingRatePaletteNV *pShadingRatePalettes)
{
    struct vkCmdSetViewportShadingRatePaletteNV_params params;
    params.commandBuffer = commandBuffer;
    params.firstViewport = firstViewport;
    params.viewportCount = viewportCount;
    params.pShadingRatePalettes = pShadingRatePalettes;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetViewportShadingRatePaletteNV, &params);
}

void WINAPI vkCmdSetViewportSwizzleNV(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkViewportSwizzleNV *pViewportSwizzles)
{
    struct vkCmdSetViewportSwizzleNV_params params;
    params.commandBuffer = commandBuffer;
    params.firstViewport = firstViewport;
    params.viewportCount = viewportCount;
    params.pViewportSwizzles = pViewportSwizzles;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetViewportSwizzleNV, &params);
}

void WINAPI vkCmdSetViewportWScalingEnableNV(VkCommandBuffer commandBuffer, VkBool32 viewportWScalingEnable)
{
    struct vkCmdSetViewportWScalingEnableNV_params params;
    params.commandBuffer = commandBuffer;
    params.viewportWScalingEnable = viewportWScalingEnable;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetViewportWScalingEnableNV, &params);
}

void WINAPI vkCmdSetViewportWScalingNV(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkViewportWScalingNV *pViewportWScalings)
{
    struct vkCmdSetViewportWScalingNV_params params;
    params.commandBuffer = commandBuffer;
    params.firstViewport = firstViewport;
    params.viewportCount = viewportCount;
    params.pViewportWScalings = pViewportWScalings;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetViewportWScalingNV, &params);
}

void WINAPI vkCmdSetViewportWithCount(VkCommandBuffer commandBuffer, uint32_t viewportCount, const VkViewport *pViewports)
{
    struct vkCmdSetViewportWithCount_params params;
    params.commandBuffer = commandBuffer;
    params.viewportCount = viewportCount;
    params.pViewports = pViewports;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetViewportWithCount, &params);
}

void WINAPI vkCmdSetViewportWithCountEXT(VkCommandBuffer commandBuffer, uint32_t viewportCount, const VkViewport *pViewports)
{
    struct vkCmdSetViewportWithCountEXT_params params;
    params.commandBuffer = commandBuffer;
    params.viewportCount = viewportCount;
    params.pViewports = pViewports;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSetViewportWithCountEXT, &params);
}

void WINAPI vkCmdSubpassShadingHUAWEI(VkCommandBuffer commandBuffer)
{
    struct vkCmdSubpassShadingHUAWEI_params params;
    params.commandBuffer = commandBuffer;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdSubpassShadingHUAWEI, &params);
}

void WINAPI vkCmdTraceRaysIndirect2KHR(VkCommandBuffer commandBuffer, VkDeviceAddress indirectDeviceAddress)
{
    struct vkCmdTraceRaysIndirect2KHR_params params;
    params.commandBuffer = commandBuffer;
    params.indirectDeviceAddress = indirectDeviceAddress;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdTraceRaysIndirect2KHR, &params);
}

void WINAPI vkCmdTraceRaysIndirectKHR(VkCommandBuffer commandBuffer, const VkStridedDeviceAddressRegionKHR *pRaygenShaderBindingTable, const VkStridedDeviceAddressRegionKHR *pMissShaderBindingTable, const VkStridedDeviceAddressRegionKHR *pHitShaderBindingTable, const VkStridedDeviceAddressRegionKHR *pCallableShaderBindingTable, VkDeviceAddress indirectDeviceAddress)
{
    struct vkCmdTraceRaysIndirectKHR_params params;
    params.commandBuffer = commandBuffer;
    params.pRaygenShaderBindingTable = pRaygenShaderBindingTable;
    params.pMissShaderBindingTable = pMissShaderBindingTable;
    params.pHitShaderBindingTable = pHitShaderBindingTable;
    params.pCallableShaderBindingTable = pCallableShaderBindingTable;
    params.indirectDeviceAddress = indirectDeviceAddress;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdTraceRaysIndirectKHR, &params);
}

void WINAPI vkCmdTraceRaysKHR(VkCommandBuffer commandBuffer, const VkStridedDeviceAddressRegionKHR *pRaygenShaderBindingTable, const VkStridedDeviceAddressRegionKHR *pMissShaderBindingTable, const VkStridedDeviceAddressRegionKHR *pHitShaderBindingTable, const VkStridedDeviceAddressRegionKHR *pCallableShaderBindingTable, uint32_t width, uint32_t height, uint32_t depth)
{
    struct vkCmdTraceRaysKHR_params params;
    params.commandBuffer = commandBuffer;
    params.pRaygenShaderBindingTable = pRaygenShaderBindingTable;
    params.pMissShaderBindingTable = pMissShaderBindingTable;
    params.pHitShaderBindingTable = pHitShaderBindingTable;
    params.pCallableShaderBindingTable = pCallableShaderBindingTable;
    params.width = width;
    params.height = height;
    params.depth = depth;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdTraceRaysKHR, &params);
}

void WINAPI vkCmdTraceRaysNV(VkCommandBuffer commandBuffer, VkBuffer raygenShaderBindingTableBuffer, VkDeviceSize raygenShaderBindingOffset, VkBuffer missShaderBindingTableBuffer, VkDeviceSize missShaderBindingOffset, VkDeviceSize missShaderBindingStride, VkBuffer hitShaderBindingTableBuffer, VkDeviceSize hitShaderBindingOffset, VkDeviceSize hitShaderBindingStride, VkBuffer callableShaderBindingTableBuffer, VkDeviceSize callableShaderBindingOffset, VkDeviceSize callableShaderBindingStride, uint32_t width, uint32_t height, uint32_t depth)
{
    struct vkCmdTraceRaysNV_params params;
    params.commandBuffer = commandBuffer;
    params.raygenShaderBindingTableBuffer = raygenShaderBindingTableBuffer;
    params.raygenShaderBindingOffset = raygenShaderBindingOffset;
    params.missShaderBindingTableBuffer = missShaderBindingTableBuffer;
    params.missShaderBindingOffset = missShaderBindingOffset;
    params.missShaderBindingStride = missShaderBindingStride;
    params.hitShaderBindingTableBuffer = hitShaderBindingTableBuffer;
    params.hitShaderBindingOffset = hitShaderBindingOffset;
    params.hitShaderBindingStride = hitShaderBindingStride;
    params.callableShaderBindingTableBuffer = callableShaderBindingTableBuffer;
    params.callableShaderBindingOffset = callableShaderBindingOffset;
    params.callableShaderBindingStride = callableShaderBindingStride;
    params.width = width;
    params.height = height;
    params.depth = depth;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdTraceRaysNV, &params);
}

void WINAPI vkCmdUpdateBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void *pData)
{
    struct vkCmdUpdateBuffer_params params;
    params.commandBuffer = commandBuffer;
    params.dstBuffer = dstBuffer;
    params.dstOffset = dstOffset;
    params.dataSize = dataSize;
    params.pData = pData;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdUpdateBuffer, &params);
}

void WINAPI vkCmdWaitEvents(VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent *pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier *pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier *pImageMemoryBarriers)
{
    struct vkCmdWaitEvents_params params;
    params.commandBuffer = commandBuffer;
    params.eventCount = eventCount;
    params.pEvents = pEvents;
    params.srcStageMask = srcStageMask;
    params.dstStageMask = dstStageMask;
    params.memoryBarrierCount = memoryBarrierCount;
    params.pMemoryBarriers = pMemoryBarriers;
    params.bufferMemoryBarrierCount = bufferMemoryBarrierCount;
    params.pBufferMemoryBarriers = pBufferMemoryBarriers;
    params.imageMemoryBarrierCount = imageMemoryBarrierCount;
    params.pImageMemoryBarriers = pImageMemoryBarriers;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdWaitEvents, &params);
}

void WINAPI vkCmdWaitEvents2(VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent *pEvents, const VkDependencyInfo *pDependencyInfos)
{
    struct vkCmdWaitEvents2_params params;
    params.commandBuffer = commandBuffer;
    params.eventCount = eventCount;
    params.pEvents = pEvents;
    params.pDependencyInfos = pDependencyInfos;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdWaitEvents2, &params);
}

void WINAPI vkCmdWaitEvents2KHR(VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent *pEvents, const VkDependencyInfo *pDependencyInfos)
{
    struct vkCmdWaitEvents2KHR_params params;
    params.commandBuffer = commandBuffer;
    params.eventCount = eventCount;
    params.pEvents = pEvents;
    params.pDependencyInfos = pDependencyInfos;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdWaitEvents2KHR, &params);
}

void WINAPI vkCmdWriteAccelerationStructuresPropertiesKHR(VkCommandBuffer commandBuffer, uint32_t accelerationStructureCount, const VkAccelerationStructureKHR *pAccelerationStructures, VkQueryType queryType, VkQueryPool queryPool, uint32_t firstQuery)
{
    struct vkCmdWriteAccelerationStructuresPropertiesKHR_params params;
    params.commandBuffer = commandBuffer;
    params.accelerationStructureCount = accelerationStructureCount;
    params.pAccelerationStructures = pAccelerationStructures;
    params.queryType = queryType;
    params.queryPool = queryPool;
    params.firstQuery = firstQuery;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdWriteAccelerationStructuresPropertiesKHR, &params);
}

void WINAPI vkCmdWriteAccelerationStructuresPropertiesNV(VkCommandBuffer commandBuffer, uint32_t accelerationStructureCount, const VkAccelerationStructureNV *pAccelerationStructures, VkQueryType queryType, VkQueryPool queryPool, uint32_t firstQuery)
{
    struct vkCmdWriteAccelerationStructuresPropertiesNV_params params;
    params.commandBuffer = commandBuffer;
    params.accelerationStructureCount = accelerationStructureCount;
    params.pAccelerationStructures = pAccelerationStructures;
    params.queryType = queryType;
    params.queryPool = queryPool;
    params.firstQuery = firstQuery;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdWriteAccelerationStructuresPropertiesNV, &params);
}

void WINAPI vkCmdWriteBufferMarker2AMD(VkCommandBuffer commandBuffer, VkPipelineStageFlags2 stage, VkBuffer dstBuffer, VkDeviceSize dstOffset, uint32_t marker)
{
    struct vkCmdWriteBufferMarker2AMD_params params;
    params.commandBuffer = commandBuffer;
    params.stage = stage;
    params.dstBuffer = dstBuffer;
    params.dstOffset = dstOffset;
    params.marker = marker;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdWriteBufferMarker2AMD, &params);
}

void WINAPI vkCmdWriteBufferMarkerAMD(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, VkBuffer dstBuffer, VkDeviceSize dstOffset, uint32_t marker)
{
    struct vkCmdWriteBufferMarkerAMD_params params;
    params.commandBuffer = commandBuffer;
    params.pipelineStage = pipelineStage;
    params.dstBuffer = dstBuffer;
    params.dstOffset = dstOffset;
    params.marker = marker;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdWriteBufferMarkerAMD, &params);
}

void WINAPI vkCmdWriteMicromapsPropertiesEXT(VkCommandBuffer commandBuffer, uint32_t micromapCount, const VkMicromapEXT *pMicromaps, VkQueryType queryType, VkQueryPool queryPool, uint32_t firstQuery)
{
    struct vkCmdWriteMicromapsPropertiesEXT_params params;
    params.commandBuffer = commandBuffer;
    params.micromapCount = micromapCount;
    params.pMicromaps = pMicromaps;
    params.queryType = queryType;
    params.queryPool = queryPool;
    params.firstQuery = firstQuery;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdWriteMicromapsPropertiesEXT, &params);
}

void WINAPI vkCmdWriteTimestamp(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool, uint32_t query)
{
    struct vkCmdWriteTimestamp_params params;
    params.commandBuffer = commandBuffer;
    params.pipelineStage = pipelineStage;
    params.queryPool = queryPool;
    params.query = query;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdWriteTimestamp, &params);
}

void WINAPI vkCmdWriteTimestamp2(VkCommandBuffer commandBuffer, VkPipelineStageFlags2 stage, VkQueryPool queryPool, uint32_t query)
{
    struct vkCmdWriteTimestamp2_params params;
    params.commandBuffer = commandBuffer;
    params.stage = stage;
    params.queryPool = queryPool;
    params.query = query;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdWriteTimestamp2, &params);
}

void WINAPI vkCmdWriteTimestamp2KHR(VkCommandBuffer commandBuffer, VkPipelineStageFlags2 stage, VkQueryPool queryPool, uint32_t query)
{
    struct vkCmdWriteTimestamp2KHR_params params;
    params.commandBuffer = commandBuffer;
    params.stage = stage;
    params.queryPool = queryPool;
    params.query = query;
    p_vk_direct_unix_call(unix_handle, unix_vkCmdWriteTimestamp2KHR, &params);
}

VkResult WINAPI vkCompileDeferredNV(VkDevice device, VkPipeline pipeline, uint32_t shader)
{
    struct vkCompileDeferredNV_params params;
    params.device = device;
    params.pipeline = pipeline;
    params.shader = shader;
    vk_unix_call(unix_vkCompileDeferredNV, &params);
    return params.result;
}

VkResult WINAPI vkCopyAccelerationStructureKHR(VkDevice device, VkDeferredOperationKHR deferredOperation, const VkCopyAccelerationStructureInfoKHR *pInfo)
{
    struct vkCopyAccelerationStructureKHR_params params;
    params.device = device;
    params.deferredOperation = deferredOperation;
    params.pInfo = pInfo;
    vk_unix_call(unix_vkCopyAccelerationStructureKHR, &params);
    return params.result;
}

VkResult WINAPI vkCopyAccelerationStructureToMemoryKHR(VkDevice device, VkDeferredOperationKHR deferredOperation, const VkCopyAccelerationStructureToMemoryInfoKHR *pInfo)
{
    struct vkCopyAccelerationStructureToMemoryKHR_params params;
    params.device = device;
    params.deferredOperation = deferredOperation;
    params.pInfo = pInfo;
    vk_unix_call(unix_vkCopyAccelerationStructureToMemoryKHR, &params);
    return params.result;
}

VkResult WINAPI vkCopyMemoryToAccelerationStructureKHR(VkDevice device, VkDeferredOperationKHR deferredOperation, const VkCopyMemoryToAccelerationStructureInfoKHR *pInfo)
{
    struct vkCopyMemoryToAccelerationStructureKHR_params params;
    params.device = device;
    params.deferredOperation = deferredOperation;
    params.pInfo = pInfo;
    vk_unix_call(unix_vkCopyMemoryToAccelerationStructureKHR, &params);
    return params.result;
}

VkResult WINAPI vkCopyMemoryToMicromapEXT(VkDevice device, VkDeferredOperationKHR deferredOperation, const VkCopyMemoryToMicromapInfoEXT *pInfo)
{
    struct vkCopyMemoryToMicromapEXT_params params;
    params.device = device;
    params.deferredOperation = deferredOperation;
    params.pInfo = pInfo;
    vk_unix_call(unix_vkCopyMemoryToMicromapEXT, &params);
    return params.result;
}

VkResult WINAPI vkCopyMicromapEXT(VkDevice device, VkDeferredOperationKHR deferredOperation, const VkCopyMicromapInfoEXT *pInfo)
{
    struct vkCopyMicromapEXT_params params;
    params.device = device;
    params.deferredOperation = deferredOperation;
    params.pInfo = pInfo;
    vk_unix_call(unix_vkCopyMicromapEXT, &params);
    return params.result;
}

VkResult WINAPI vkCopyMicromapToMemoryEXT(VkDevice device, VkDeferredOperationKHR deferredOperation, const VkCopyMicromapToMemoryInfoEXT *pInfo)
{
    struct vkCopyMicromapToMemoryEXT_params params;
    params.device = device;
    params.deferredOperation = deferredOperation;
    params.pInfo = pInfo;
    vk_unix_call(unix_vkCopyMicromapToMemoryEXT, &params);
    return params.result;
}

VkResult WINAPI vkCreateAccelerationStructureKHR(VkDevice device, const VkAccelerationStructureCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkAccelerationStructureKHR *pAccelerationStructure)
{
    struct vkCreateAccelerationStructureKHR_params params;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pAccelerationStructure = pAccelerationStructure;
    vk_unix_call(unix_vkCreateAccelerationStructureKHR, &params);
    return params.result;
}

VkResult WINAPI vkCreateAccelerationStructureNV(VkDevice device, const VkAccelerationStructureCreateInfoNV *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkAccelerationStructureNV *pAccelerationStructure)
{
    struct vkCreateAccelerationStructureNV_params params;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pAccelerationStructure = pAccelerationStructure;
    vk_unix_call(unix_vkCreateAccelerationStructureNV, &params);
    return params.result;
}

VkResult WINAPI vkCreateBuffer(VkDevice device, const VkBufferCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkBuffer *pBuffer)
{
    struct vkCreateBuffer_params params;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pBuffer = pBuffer;
    vk_unix_call(unix_vkCreateBuffer, &params);
    return params.result;
}

VkResult WINAPI vkCreateBufferView(VkDevice device, const VkBufferViewCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkBufferView *pView)
{
    struct vkCreateBufferView_params params;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pView = pView;
    vk_unix_call(unix_vkCreateBufferView, &params);
    return params.result;
}

VkResult WINAPI vkCreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkComputePipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines)
{
    struct vkCreateComputePipelines_params params;
    params.device = device;
    params.pipelineCache = pipelineCache;
    params.createInfoCount = createInfoCount;
    params.pCreateInfos = pCreateInfos;
    params.pAllocator = pAllocator;
    params.pPipelines = pPipelines;
    vk_unix_call(unix_vkCreateComputePipelines, &params);
    return params.result;
}

VkResult WINAPI vkCreateCuFunctionNVX(VkDevice device, const VkCuFunctionCreateInfoNVX *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkCuFunctionNVX *pFunction)
{
    struct vkCreateCuFunctionNVX_params params;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pFunction = pFunction;
    vk_unix_call(unix_vkCreateCuFunctionNVX, &params);
    return params.result;
}

VkResult WINAPI vkCreateCuModuleNVX(VkDevice device, const VkCuModuleCreateInfoNVX *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkCuModuleNVX *pModule)
{
    struct vkCreateCuModuleNVX_params params;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pModule = pModule;
    vk_unix_call(unix_vkCreateCuModuleNVX, &params);
    return params.result;
}

VkResult WINAPI vkCreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugReportCallbackEXT *pCallback)
{
    struct vkCreateDebugReportCallbackEXT_params params;
    params.instance = instance;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pCallback = pCallback;
    vk_unix_call(unix_vkCreateDebugReportCallbackEXT, &params);
    return params.result;
}

VkResult WINAPI vkCreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pMessenger)
{
    struct vkCreateDebugUtilsMessengerEXT_params params;
    params.instance = instance;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pMessenger = pMessenger;
    vk_unix_call(unix_vkCreateDebugUtilsMessengerEXT, &params);
    return params.result;
}

VkResult WINAPI vkCreateDeferredOperationKHR(VkDevice device, const VkAllocationCallbacks *pAllocator, VkDeferredOperationKHR *pDeferredOperation)
{
    struct vkCreateDeferredOperationKHR_params params;
    params.device = device;
    params.pAllocator = pAllocator;
    params.pDeferredOperation = pDeferredOperation;
    vk_unix_call(unix_vkCreateDeferredOperationKHR, &params);
    return params.result;
}

VkResult WINAPI vkCreateDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDescriptorPool *pDescriptorPool)
{
    struct vkCreateDescriptorPool_params params;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pDescriptorPool = pDescriptorPool;
    vk_unix_call(unix_vkCreateDescriptorPool, &params);
    return params.result;
}

VkResult WINAPI vkCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDescriptorSetLayout *pSetLayout)
{
    struct vkCreateDescriptorSetLayout_params params;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pSetLayout = pSetLayout;
    vk_unix_call(unix_vkCreateDescriptorSetLayout, &params);
    return params.result;
}

VkResult WINAPI vkCreateDescriptorUpdateTemplate(VkDevice device, const VkDescriptorUpdateTemplateCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDescriptorUpdateTemplate *pDescriptorUpdateTemplate)
{
    struct vkCreateDescriptorUpdateTemplate_params params;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pDescriptorUpdateTemplate = pDescriptorUpdateTemplate;
    vk_unix_call(unix_vkCreateDescriptorUpdateTemplate, &params);
    return params.result;
}

VkResult WINAPI vkCreateDescriptorUpdateTemplateKHR(VkDevice device, const VkDescriptorUpdateTemplateCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDescriptorUpdateTemplate *pDescriptorUpdateTemplate)
{
    struct vkCreateDescriptorUpdateTemplateKHR_params params;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pDescriptorUpdateTemplate = pDescriptorUpdateTemplate;
    vk_unix_call(unix_vkCreateDescriptorUpdateTemplateKHR, &params);
    return params.result;
}

VkResult WINAPI vkCreateEvent(VkDevice device, const VkEventCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkEvent *pEvent)
{
    struct vkCreateEvent_params params;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pEvent = pEvent;
    vk_unix_call(unix_vkCreateEvent, &params);
    return params.result;
}

VkResult WINAPI vkCreateFence(VkDevice device, const VkFenceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkFence *pFence)
{
    struct vkCreateFence_params params;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pFence = pFence;
    vk_unix_call(unix_vkCreateFence, &params);
    return params.result;
}

VkResult WINAPI vkCreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkFramebuffer *pFramebuffer)
{
    struct vkCreateFramebuffer_params params;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pFramebuffer = pFramebuffer;
    vk_unix_call(unix_vkCreateFramebuffer, &params);
    return params.result;
}

VkResult WINAPI vkCreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines)
{
    struct vkCreateGraphicsPipelines_params params;
    params.device = device;
    params.pipelineCache = pipelineCache;
    params.createInfoCount = createInfoCount;
    params.pCreateInfos = pCreateInfos;
    params.pAllocator = pAllocator;
    params.pPipelines = pPipelines;
    vk_unix_call(unix_vkCreateGraphicsPipelines, &params);
    return params.result;
}

VkResult WINAPI vkCreateImage(VkDevice device, const VkImageCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkImage *pImage)
{
    struct vkCreateImage_params params;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pImage = pImage;
    vk_unix_call(unix_vkCreateImage, &params);
    return params.result;
}

VkResult WINAPI vkCreateImageView(VkDevice device, const VkImageViewCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkImageView *pView)
{
    struct vkCreateImageView_params params;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pView = pView;
    vk_unix_call(unix_vkCreateImageView, &params);
    return params.result;
}

VkResult WINAPI vkCreateIndirectCommandsLayoutNV(VkDevice device, const VkIndirectCommandsLayoutCreateInfoNV *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkIndirectCommandsLayoutNV *pIndirectCommandsLayout)
{
    struct vkCreateIndirectCommandsLayoutNV_params params;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pIndirectCommandsLayout = pIndirectCommandsLayout;
    vk_unix_call(unix_vkCreateIndirectCommandsLayoutNV, &params);
    return params.result;
}

VkResult WINAPI vkCreateMicromapEXT(VkDevice device, const VkMicromapCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkMicromapEXT *pMicromap)
{
    struct vkCreateMicromapEXT_params params;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pMicromap = pMicromap;
    vk_unix_call(unix_vkCreateMicromapEXT, &params);
    return params.result;
}

VkResult WINAPI vkCreateOpticalFlowSessionNV(VkDevice device, const VkOpticalFlowSessionCreateInfoNV *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkOpticalFlowSessionNV *pSession)
{
    struct vkCreateOpticalFlowSessionNV_params params;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pSession = pSession;
    vk_unix_call(unix_vkCreateOpticalFlowSessionNV, &params);
    return params.result;
}

VkResult WINAPI vkCreatePipelineCache(VkDevice device, const VkPipelineCacheCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkPipelineCache *pPipelineCache)
{
    struct vkCreatePipelineCache_params params;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pPipelineCache = pPipelineCache;
    vk_unix_call(unix_vkCreatePipelineCache, &params);
    return params.result;
}

VkResult WINAPI vkCreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkPipelineLayout *pPipelineLayout)
{
    struct vkCreatePipelineLayout_params params;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pPipelineLayout = pPipelineLayout;
    vk_unix_call(unix_vkCreatePipelineLayout, &params);
    return params.result;
}

VkResult WINAPI vkCreatePrivateDataSlot(VkDevice device, const VkPrivateDataSlotCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkPrivateDataSlot *pPrivateDataSlot)
{
    struct vkCreatePrivateDataSlot_params params;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pPrivateDataSlot = pPrivateDataSlot;
    vk_unix_call(unix_vkCreatePrivateDataSlot, &params);
    return params.result;
}

VkResult WINAPI vkCreatePrivateDataSlotEXT(VkDevice device, const VkPrivateDataSlotCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkPrivateDataSlot *pPrivateDataSlot)
{
    struct vkCreatePrivateDataSlotEXT_params params;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pPrivateDataSlot = pPrivateDataSlot;
    vk_unix_call(unix_vkCreatePrivateDataSlotEXT, &params);
    return params.result;
}

VkResult WINAPI vkCreateQueryPool(VkDevice device, const VkQueryPoolCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkQueryPool *pQueryPool)
{
    struct vkCreateQueryPool_params params;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pQueryPool = pQueryPool;
    vk_unix_call(unix_vkCreateQueryPool, &params);
    return params.result;
}

VkResult WINAPI vkCreateRayTracingPipelinesKHR(VkDevice device, VkDeferredOperationKHR deferredOperation, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkRayTracingPipelineCreateInfoKHR *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines)
{
    struct vkCreateRayTracingPipelinesKHR_params params;
    params.device = device;
    params.deferredOperation = deferredOperation;
    params.pipelineCache = pipelineCache;
    params.createInfoCount = createInfoCount;
    params.pCreateInfos = pCreateInfos;
    params.pAllocator = pAllocator;
    params.pPipelines = pPipelines;
    vk_unix_call(unix_vkCreateRayTracingPipelinesKHR, &params);
    return params.result;
}

VkResult WINAPI vkCreateRayTracingPipelinesNV(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkRayTracingPipelineCreateInfoNV *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines)
{
    struct vkCreateRayTracingPipelinesNV_params params;
    params.device = device;
    params.pipelineCache = pipelineCache;
    params.createInfoCount = createInfoCount;
    params.pCreateInfos = pCreateInfos;
    params.pAllocator = pAllocator;
    params.pPipelines = pPipelines;
    vk_unix_call(unix_vkCreateRayTracingPipelinesNV, &params);
    return params.result;
}

VkResult WINAPI vkCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkRenderPass *pRenderPass)
{
    struct vkCreateRenderPass_params params;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pRenderPass = pRenderPass;
    vk_unix_call(unix_vkCreateRenderPass, &params);
    return params.result;
}

VkResult WINAPI vkCreateRenderPass2(VkDevice device, const VkRenderPassCreateInfo2 *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkRenderPass *pRenderPass)
{
    struct vkCreateRenderPass2_params params;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pRenderPass = pRenderPass;
    vk_unix_call(unix_vkCreateRenderPass2, &params);
    return params.result;
}

VkResult WINAPI vkCreateRenderPass2KHR(VkDevice device, const VkRenderPassCreateInfo2 *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkRenderPass *pRenderPass)
{
    struct vkCreateRenderPass2KHR_params params;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pRenderPass = pRenderPass;
    vk_unix_call(unix_vkCreateRenderPass2KHR, &params);
    return params.result;
}

VkResult WINAPI vkCreateSampler(VkDevice device, const VkSamplerCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSampler *pSampler)
{
    struct vkCreateSampler_params params;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pSampler = pSampler;
    vk_unix_call(unix_vkCreateSampler, &params);
    return params.result;
}

VkResult WINAPI vkCreateSamplerYcbcrConversion(VkDevice device, const VkSamplerYcbcrConversionCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSamplerYcbcrConversion *pYcbcrConversion)
{
    struct vkCreateSamplerYcbcrConversion_params params;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pYcbcrConversion = pYcbcrConversion;
    vk_unix_call(unix_vkCreateSamplerYcbcrConversion, &params);
    return params.result;
}

VkResult WINAPI vkCreateSamplerYcbcrConversionKHR(VkDevice device, const VkSamplerYcbcrConversionCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSamplerYcbcrConversion *pYcbcrConversion)
{
    struct vkCreateSamplerYcbcrConversionKHR_params params;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pYcbcrConversion = pYcbcrConversion;
    vk_unix_call(unix_vkCreateSamplerYcbcrConversionKHR, &params);
    return params.result;
}

VkResult WINAPI vkCreateSemaphore(VkDevice device, const VkSemaphoreCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSemaphore *pSemaphore)
{
    struct vkCreateSemaphore_params params;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pSemaphore = pSemaphore;
    vk_unix_call(unix_vkCreateSemaphore, &params);
    return params.result;
}

VkResult WINAPI vkCreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkShaderModule *pShaderModule)
{
    struct vkCreateShaderModule_params params;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pShaderModule = pShaderModule;
    vk_unix_call(unix_vkCreateShaderModule, &params);
    return params.result;
}

VkResult WINAPI vkCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSwapchainKHR *pSwapchain)
{
    struct vkCreateSwapchainKHR_params params;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pSwapchain = pSwapchain;
    vk_unix_call(unix_vkCreateSwapchainKHR, &params);
    return params.result;
}

VkResult WINAPI vkCreateValidationCacheEXT(VkDevice device, const VkValidationCacheCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkValidationCacheEXT *pValidationCache)
{
    struct vkCreateValidationCacheEXT_params params;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pValidationCache = pValidationCache;
    vk_unix_call(unix_vkCreateValidationCacheEXT, &params);
    return params.result;
}

VkResult WINAPI vkCreateWin32SurfaceKHR(VkInstance instance, const VkWin32SurfaceCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface)
{
    struct vkCreateWin32SurfaceKHR_params params;
    params.instance = instance;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pSurface = pSurface;
    vk_unix_call(unix_vkCreateWin32SurfaceKHR, &params);
    return params.result;
}

VkResult WINAPI vkDebugMarkerSetObjectNameEXT(VkDevice device, const VkDebugMarkerObjectNameInfoEXT *pNameInfo)
{
    struct vkDebugMarkerSetObjectNameEXT_params params;
    params.device = device;
    params.pNameInfo = pNameInfo;
    vk_unix_call(unix_vkDebugMarkerSetObjectNameEXT, &params);
    return params.result;
}

VkResult WINAPI vkDebugMarkerSetObjectTagEXT(VkDevice device, const VkDebugMarkerObjectTagInfoEXT *pTagInfo)
{
    struct vkDebugMarkerSetObjectTagEXT_params params;
    params.device = device;
    params.pTagInfo = pTagInfo;
    vk_unix_call(unix_vkDebugMarkerSetObjectTagEXT, &params);
    return params.result;
}

void WINAPI vkDebugReportMessageEXT(VkInstance instance, VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char *pLayerPrefix, const char *pMessage)
{
    struct vkDebugReportMessageEXT_params params;
    params.instance = instance;
    params.flags = flags;
    params.objectType = objectType;
    params.object = object;
    params.location = location;
    params.messageCode = messageCode;
    params.pLayerPrefix = pLayerPrefix;
    params.pMessage = pMessage;
    vk_unix_call(unix_vkDebugReportMessageEXT, &params);
}

VkResult WINAPI vkDeferredOperationJoinKHR(VkDevice device, VkDeferredOperationKHR operation)
{
    struct vkDeferredOperationJoinKHR_params params;
    params.device = device;
    params.operation = operation;
    vk_unix_call(unix_vkDeferredOperationJoinKHR, &params);
    return params.result;
}

void WINAPI vkDestroyAccelerationStructureKHR(VkDevice device, VkAccelerationStructureKHR accelerationStructure, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyAccelerationStructureKHR_params params;
    params.device = device;
    params.accelerationStructure = accelerationStructure;
    params.pAllocator = pAllocator;
    vk_unix_call(unix_vkDestroyAccelerationStructureKHR, &params);
}

void WINAPI vkDestroyAccelerationStructureNV(VkDevice device, VkAccelerationStructureNV accelerationStructure, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyAccelerationStructureNV_params params;
    params.device = device;
    params.accelerationStructure = accelerationStructure;
    params.pAllocator = pAllocator;
    vk_unix_call(unix_vkDestroyAccelerationStructureNV, &params);
}

void WINAPI vkDestroyBuffer(VkDevice device, VkBuffer buffer, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyBuffer_params params;
    params.device = device;
    params.buffer = buffer;
    params.pAllocator = pAllocator;
    vk_unix_call(unix_vkDestroyBuffer, &params);
}

void WINAPI vkDestroyBufferView(VkDevice device, VkBufferView bufferView, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyBufferView_params params;
    params.device = device;
    params.bufferView = bufferView;
    params.pAllocator = pAllocator;
    vk_unix_call(unix_vkDestroyBufferView, &params);
}

void WINAPI vkDestroyCuFunctionNVX(VkDevice device, VkCuFunctionNVX function, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyCuFunctionNVX_params params;
    params.device = device;
    params.function = function;
    params.pAllocator = pAllocator;
    vk_unix_call(unix_vkDestroyCuFunctionNVX, &params);
}

void WINAPI vkDestroyCuModuleNVX(VkDevice device, VkCuModuleNVX module, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyCuModuleNVX_params params;
    params.device = device;
    params.module = module;
    params.pAllocator = pAllocator;
    vk_unix_call(unix_vkDestroyCuModuleNVX, &params);
}

void WINAPI vkDestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyDebugReportCallbackEXT_params params;
    params.instance = instance;
    params.callback = callback;
    params.pAllocator = pAllocator;
    vk_unix_call(unix_vkDestroyDebugReportCallbackEXT, &params);
}

void WINAPI vkDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyDebugUtilsMessengerEXT_params params;
    params.instance = instance;
    params.messenger = messenger;
    params.pAllocator = pAllocator;
    vk_unix_call(unix_vkDestroyDebugUtilsMessengerEXT, &params);
}

void WINAPI vkDestroyDeferredOperationKHR(VkDevice device, VkDeferredOperationKHR operation, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyDeferredOperationKHR_params params;
    params.device = device;
    params.operation = operation;
    params.pAllocator = pAllocator;
    vk_unix_call(unix_vkDestroyDeferredOperationKHR, &params);
}

void WINAPI vkDestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyDescriptorPool_params params;
    params.device = device;
    params.descriptorPool = descriptorPool;
    params.pAllocator = pAllocator;
    vk_unix_call(unix_vkDestroyDescriptorPool, &params);
}

void WINAPI vkDestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyDescriptorSetLayout_params params;
    params.device = device;
    params.descriptorSetLayout = descriptorSetLayout;
    params.pAllocator = pAllocator;
    vk_unix_call(unix_vkDestroyDescriptorSetLayout, &params);
}

void WINAPI vkDestroyDescriptorUpdateTemplate(VkDevice device, VkDescriptorUpdateTemplate descriptorUpdateTemplate, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyDescriptorUpdateTemplate_params params;
    params.device = device;
    params.descriptorUpdateTemplate = descriptorUpdateTemplate;
    params.pAllocator = pAllocator;
    vk_unix_call(unix_vkDestroyDescriptorUpdateTemplate, &params);
}

void WINAPI vkDestroyDescriptorUpdateTemplateKHR(VkDevice device, VkDescriptorUpdateTemplate descriptorUpdateTemplate, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyDescriptorUpdateTemplateKHR_params params;
    params.device = device;
    params.descriptorUpdateTemplate = descriptorUpdateTemplate;
    params.pAllocator = pAllocator;
    vk_unix_call(unix_vkDestroyDescriptorUpdateTemplateKHR, &params);
}

void WINAPI vkDestroyEvent(VkDevice device, VkEvent event, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyEvent_params params;
    params.device = device;
    params.event = event;
    params.pAllocator = pAllocator;
    vk_unix_call(unix_vkDestroyEvent, &params);
}

void WINAPI vkDestroyFence(VkDevice device, VkFence fence, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyFence_params params;
    params.device = device;
    params.fence = fence;
    params.pAllocator = pAllocator;
    vk_unix_call(unix_vkDestroyFence, &params);
}

void WINAPI vkDestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyFramebuffer_params params;
    params.device = device;
    params.framebuffer = framebuffer;
    params.pAllocator = pAllocator;
    vk_unix_call(unix_vkDestroyFramebuffer, &params);
}

void WINAPI vkDestroyImage(VkDevice device, VkImage image, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyImage_params params;
    params.device = device;
    params.image = image;
    params.pAllocator = pAllocator;
    vk_unix_call(unix_vkDestroyImage, &params);
}

void WINAPI vkDestroyImageView(VkDevice device, VkImageView imageView, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyImageView_params params;
    params.device = device;
    params.imageView = imageView;
    params.pAllocator = pAllocator;
    vk_unix_call(unix_vkDestroyImageView, &params);
}

void WINAPI vkDestroyIndirectCommandsLayoutNV(VkDevice device, VkIndirectCommandsLayoutNV indirectCommandsLayout, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyIndirectCommandsLayoutNV_params params;
    params.device = device;
    params.indirectCommandsLayout = indirectCommandsLayout;
    params.pAllocator = pAllocator;
    vk_unix_call(unix_vkDestroyIndirectCommandsLayoutNV, &params);
}

void WINAPI vkDestroyMicromapEXT(VkDevice device, VkMicromapEXT micromap, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyMicromapEXT_params params;
    params.device = device;
    params.micromap = micromap;
    params.pAllocator = pAllocator;
    vk_unix_call(unix_vkDestroyMicromapEXT, &params);
}

void WINAPI vkDestroyOpticalFlowSessionNV(VkDevice device, VkOpticalFlowSessionNV session, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyOpticalFlowSessionNV_params params;
    params.device = device;
    params.session = session;
    params.pAllocator = pAllocator;
    vk_unix_call(unix_vkDestroyOpticalFlowSessionNV, &params);
}

void WINAPI vkDestroyPipeline(VkDevice device, VkPipeline pipeline, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyPipeline_params params;
    params.device = device;
    params.pipeline = pipeline;
    params.pAllocator = pAllocator;
    vk_unix_call(unix_vkDestroyPipeline, &params);
}

void WINAPI vkDestroyPipelineCache(VkDevice device, VkPipelineCache pipelineCache, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyPipelineCache_params params;
    params.device = device;
    params.pipelineCache = pipelineCache;
    params.pAllocator = pAllocator;
    vk_unix_call(unix_vkDestroyPipelineCache, &params);
}

void WINAPI vkDestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyPipelineLayout_params params;
    params.device = device;
    params.pipelineLayout = pipelineLayout;
    params.pAllocator = pAllocator;
    vk_unix_call(unix_vkDestroyPipelineLayout, &params);
}

void WINAPI vkDestroyPrivateDataSlot(VkDevice device, VkPrivateDataSlot privateDataSlot, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyPrivateDataSlot_params params;
    params.device = device;
    params.privateDataSlot = privateDataSlot;
    params.pAllocator = pAllocator;
    vk_unix_call(unix_vkDestroyPrivateDataSlot, &params);
}

void WINAPI vkDestroyPrivateDataSlotEXT(VkDevice device, VkPrivateDataSlot privateDataSlot, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyPrivateDataSlotEXT_params params;
    params.device = device;
    params.privateDataSlot = privateDataSlot;
    params.pAllocator = pAllocator;
    vk_unix_call(unix_vkDestroyPrivateDataSlotEXT, &params);
}

void WINAPI vkDestroyQueryPool(VkDevice device, VkQueryPool queryPool, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyQueryPool_params params;
    params.device = device;
    params.queryPool = queryPool;
    params.pAllocator = pAllocator;
    vk_unix_call(unix_vkDestroyQueryPool, &params);
}

void WINAPI vkDestroyRenderPass(VkDevice device, VkRenderPass renderPass, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyRenderPass_params params;
    params.device = device;
    params.renderPass = renderPass;
    params.pAllocator = pAllocator;
    vk_unix_call(unix_vkDestroyRenderPass, &params);
}

void WINAPI vkDestroySampler(VkDevice device, VkSampler sampler, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroySampler_params params;
    params.device = device;
    params.sampler = sampler;
    params.pAllocator = pAllocator;
    vk_unix_call(unix_vkDestroySampler, &params);
}

void WINAPI vkDestroySamplerYcbcrConversion(VkDevice device, VkSamplerYcbcrConversion ycbcrConversion, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroySamplerYcbcrConversion_params params;
    params.device = device;
    params.ycbcrConversion = ycbcrConversion;
    params.pAllocator = pAllocator;
    vk_unix_call(unix_vkDestroySamplerYcbcrConversion, &params);
}

void WINAPI vkDestroySamplerYcbcrConversionKHR(VkDevice device, VkSamplerYcbcrConversion ycbcrConversion, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroySamplerYcbcrConversionKHR_params params;
    params.device = device;
    params.ycbcrConversion = ycbcrConversion;
    params.pAllocator = pAllocator;
    vk_unix_call(unix_vkDestroySamplerYcbcrConversionKHR, &params);
}

void WINAPI vkDestroySemaphore(VkDevice device, VkSemaphore semaphore, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroySemaphore_params params;
    params.device = device;
    params.semaphore = semaphore;
    params.pAllocator = pAllocator;
    vk_unix_call(unix_vkDestroySemaphore, &params);
}

void WINAPI vkDestroyShaderModule(VkDevice device, VkShaderModule shaderModule, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyShaderModule_params params;
    params.device = device;
    params.shaderModule = shaderModule;
    params.pAllocator = pAllocator;
    vk_unix_call(unix_vkDestroyShaderModule, &params);
}

void WINAPI vkDestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroySurfaceKHR_params params;
    params.instance = instance;
    params.surface = surface;
    params.pAllocator = pAllocator;
    vk_unix_call(unix_vkDestroySurfaceKHR, &params);
}

void WINAPI vkDestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroySwapchainKHR_params params;
    params.device = device;
    params.swapchain = swapchain;
    params.pAllocator = pAllocator;
    vk_unix_call(unix_vkDestroySwapchainKHR, &params);
}

void WINAPI vkDestroyValidationCacheEXT(VkDevice device, VkValidationCacheEXT validationCache, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyValidationCacheEXT_params params;
    params.device = device;
    params.validationCache = validationCache;
    params.pAllocator = pAllocator;
    vk_unix_call(unix_vkDestroyValidationCacheEXT, &params);
}

VkResult WINAPI vkDeviceWaitIdle(VkDevice device)
{
    struct vkDeviceWaitIdle_params params;
    params.device = device;
    vk_unix_call(unix_vkDeviceWaitIdle, &params);
    return params.result;
}

VkResult WINAPI vkEndCommandBuffer(VkCommandBuffer commandBuffer)
{
    struct vkEndCommandBuffer_params params;
    params.commandBuffer = commandBuffer;
    vk_unix_call(unix_vkEndCommandBuffer, &params);
    return params.result;
}

VkResult WINAPI vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char *pLayerName, uint32_t *pPropertyCount, VkExtensionProperties *pProperties)
{
    struct vkEnumerateDeviceExtensionProperties_params params;
    params.physicalDevice = physicalDevice;
    params.pLayerName = pLayerName;
    params.pPropertyCount = pPropertyCount;
    params.pProperties = pProperties;
    vk_unix_call(unix_vkEnumerateDeviceExtensionProperties, &params);
    return params.result;
}

VkResult WINAPI vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount, VkLayerProperties *pProperties)
{
    struct vkEnumerateDeviceLayerProperties_params params;
    params.physicalDevice = physicalDevice;
    params.pPropertyCount = pPropertyCount;
    params.pProperties = pProperties;
    vk_unix_call(unix_vkEnumerateDeviceLayerProperties, &params);
    return params.result;
}

VkResult WINAPI vkEnumeratePhysicalDeviceGroups(VkInstance instance, uint32_t *pPhysicalDeviceGroupCount, VkPhysicalDeviceGroupProperties *pPhysicalDeviceGroupProperties)
{
    struct vkEnumeratePhysicalDeviceGroups_params params;
    params.instance = instance;
    params.pPhysicalDeviceGroupCount = pPhysicalDeviceGroupCount;
    params.pPhysicalDeviceGroupProperties = pPhysicalDeviceGroupProperties;
    vk_unix_call(unix_vkEnumeratePhysicalDeviceGroups, &params);
    return params.result;
}

VkResult WINAPI vkEnumeratePhysicalDeviceGroupsKHR(VkInstance instance, uint32_t *pPhysicalDeviceGroupCount, VkPhysicalDeviceGroupProperties *pPhysicalDeviceGroupProperties)
{
    struct vkEnumeratePhysicalDeviceGroupsKHR_params params;
    params.instance = instance;
    params.pPhysicalDeviceGroupCount = pPhysicalDeviceGroupCount;
    params.pPhysicalDeviceGroupProperties = pPhysicalDeviceGroupProperties;
    vk_unix_call(unix_vkEnumeratePhysicalDeviceGroupsKHR, &params);
    return params.result;
}

VkResult WINAPI vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, uint32_t *pCounterCount, VkPerformanceCounterKHR *pCounters, VkPerformanceCounterDescriptionKHR *pCounterDescriptions)
{
    struct vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR_params params;
    params.physicalDevice = physicalDevice;
    params.queueFamilyIndex = queueFamilyIndex;
    params.pCounterCount = pCounterCount;
    params.pCounters = pCounters;
    params.pCounterDescriptions = pCounterDescriptions;
    vk_unix_call(unix_vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR, &params);
    return params.result;
}

VkResult WINAPI vkEnumeratePhysicalDevices(VkInstance instance, uint32_t *pPhysicalDeviceCount, VkPhysicalDevice *pPhysicalDevices)
{
    struct vkEnumeratePhysicalDevices_params params;
    params.instance = instance;
    params.pPhysicalDeviceCount = pPhysicalDeviceCount;
    params.pPhysicalDevices = pPhysicalDevices;
    vk_unix_call(unix_vkEnumeratePhysicalDevices, &params);
    return params.result;
}

VkResult WINAPI vkFlushMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount, const VkMappedMemoryRange *pMemoryRanges)
{
    struct vkFlushMappedMemoryRanges_params params;
    params.device = device;
    params.memoryRangeCount = memoryRangeCount;
    params.pMemoryRanges = pMemoryRanges;
    vk_unix_call(unix_vkFlushMappedMemoryRanges, &params);
    return params.result;
}

VkResult WINAPI vkFreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t descriptorSetCount, const VkDescriptorSet *pDescriptorSets)
{
    struct vkFreeDescriptorSets_params params;
    params.device = device;
    params.descriptorPool = descriptorPool;
    params.descriptorSetCount = descriptorSetCount;
    params.pDescriptorSets = pDescriptorSets;
    vk_unix_call(unix_vkFreeDescriptorSets, &params);
    return params.result;
}

void WINAPI vkFreeMemory(VkDevice device, VkDeviceMemory memory, const VkAllocationCallbacks *pAllocator)
{
    struct vkFreeMemory_params params;
    params.device = device;
    params.memory = memory;
    params.pAllocator = pAllocator;
    vk_unix_call(unix_vkFreeMemory, &params);
}

void WINAPI vkGetAccelerationStructureBuildSizesKHR(VkDevice device, VkAccelerationStructureBuildTypeKHR buildType, const VkAccelerationStructureBuildGeometryInfoKHR *pBuildInfo, const uint32_t *pMaxPrimitiveCounts, VkAccelerationStructureBuildSizesInfoKHR *pSizeInfo)
{
    struct vkGetAccelerationStructureBuildSizesKHR_params params;
    params.device = device;
    params.buildType = buildType;
    params.pBuildInfo = pBuildInfo;
    params.pMaxPrimitiveCounts = pMaxPrimitiveCounts;
    params.pSizeInfo = pSizeInfo;
    vk_unix_call(unix_vkGetAccelerationStructureBuildSizesKHR, &params);
}

VkDeviceAddress WINAPI vkGetAccelerationStructureDeviceAddressKHR(VkDevice device, const VkAccelerationStructureDeviceAddressInfoKHR *pInfo)
{
    struct vkGetAccelerationStructureDeviceAddressKHR_params params;
    params.device = device;
    params.pInfo = pInfo;
    vk_unix_call(unix_vkGetAccelerationStructureDeviceAddressKHR, &params);
    return params.result;
}

VkResult WINAPI vkGetAccelerationStructureHandleNV(VkDevice device, VkAccelerationStructureNV accelerationStructure, size_t dataSize, void *pData)
{
    struct vkGetAccelerationStructureHandleNV_params params;
    params.device = device;
    params.accelerationStructure = accelerationStructure;
    params.dataSize = dataSize;
    params.pData = pData;
    vk_unix_call(unix_vkGetAccelerationStructureHandleNV, &params);
    return params.result;
}

void WINAPI vkGetAccelerationStructureMemoryRequirementsNV(VkDevice device, const VkAccelerationStructureMemoryRequirementsInfoNV *pInfo, VkMemoryRequirements2KHR *pMemoryRequirements)
{
    struct vkGetAccelerationStructureMemoryRequirementsNV_params params;
    params.device = device;
    params.pInfo = pInfo;
    params.pMemoryRequirements = pMemoryRequirements;
    vk_unix_call(unix_vkGetAccelerationStructureMemoryRequirementsNV, &params);
}

VkDeviceAddress WINAPI vkGetBufferDeviceAddress(VkDevice device, const VkBufferDeviceAddressInfo *pInfo)
{
    struct vkGetBufferDeviceAddress_params params;
    params.device = device;
    params.pInfo = pInfo;
    vk_unix_call(unix_vkGetBufferDeviceAddress, &params);
    return params.result;
}

VkDeviceAddress WINAPI vkGetBufferDeviceAddressEXT(VkDevice device, const VkBufferDeviceAddressInfo *pInfo)
{
    struct vkGetBufferDeviceAddressEXT_params params;
    params.device = device;
    params.pInfo = pInfo;
    vk_unix_call(unix_vkGetBufferDeviceAddressEXT, &params);
    return params.result;
}

VkDeviceAddress WINAPI vkGetBufferDeviceAddressKHR(VkDevice device, const VkBufferDeviceAddressInfo *pInfo)
{
    struct vkGetBufferDeviceAddressKHR_params params;
    params.device = device;
    params.pInfo = pInfo;
    vk_unix_call(unix_vkGetBufferDeviceAddressKHR, &params);
    return params.result;
}

void WINAPI vkGetBufferMemoryRequirements(VkDevice device, VkBuffer buffer, VkMemoryRequirements *pMemoryRequirements)
{
    struct vkGetBufferMemoryRequirements_params params;
    params.device = device;
    params.buffer = buffer;
    params.pMemoryRequirements = pMemoryRequirements;
    vk_unix_call(unix_vkGetBufferMemoryRequirements, &params);
}

void WINAPI vkGetBufferMemoryRequirements2(VkDevice device, const VkBufferMemoryRequirementsInfo2 *pInfo, VkMemoryRequirements2 *pMemoryRequirements)
{
    struct vkGetBufferMemoryRequirements2_params params;
    params.device = device;
    params.pInfo = pInfo;
    params.pMemoryRequirements = pMemoryRequirements;
    vk_unix_call(unix_vkGetBufferMemoryRequirements2, &params);
}

void WINAPI vkGetBufferMemoryRequirements2KHR(VkDevice device, const VkBufferMemoryRequirementsInfo2 *pInfo, VkMemoryRequirements2 *pMemoryRequirements)
{
    struct vkGetBufferMemoryRequirements2KHR_params params;
    params.device = device;
    params.pInfo = pInfo;
    params.pMemoryRequirements = pMemoryRequirements;
    vk_unix_call(unix_vkGetBufferMemoryRequirements2KHR, &params);
}

uint64_t WINAPI vkGetBufferOpaqueCaptureAddress(VkDevice device, const VkBufferDeviceAddressInfo *pInfo)
{
    struct vkGetBufferOpaqueCaptureAddress_params params;
    params.device = device;
    params.pInfo = pInfo;
    vk_unix_call(unix_vkGetBufferOpaqueCaptureAddress, &params);
    return params.result;
}

uint64_t WINAPI vkGetBufferOpaqueCaptureAddressKHR(VkDevice device, const VkBufferDeviceAddressInfo *pInfo)
{
    struct vkGetBufferOpaqueCaptureAddressKHR_params params;
    params.device = device;
    params.pInfo = pInfo;
    vk_unix_call(unix_vkGetBufferOpaqueCaptureAddressKHR, &params);
    return params.result;
}

VkResult WINAPI vkGetCalibratedTimestampsEXT(VkDevice device, uint32_t timestampCount, const VkCalibratedTimestampInfoEXT *pTimestampInfos, uint64_t *pTimestamps, uint64_t *pMaxDeviation)
{
    struct vkGetCalibratedTimestampsEXT_params params;
    params.device = device;
    params.timestampCount = timestampCount;
    params.pTimestampInfos = pTimestampInfos;
    params.pTimestamps = pTimestamps;
    params.pMaxDeviation = pMaxDeviation;
    vk_unix_call(unix_vkGetCalibratedTimestampsEXT, &params);
    return params.result;
}

uint32_t WINAPI vkGetDeferredOperationMaxConcurrencyKHR(VkDevice device, VkDeferredOperationKHR operation)
{
    struct vkGetDeferredOperationMaxConcurrencyKHR_params params;
    params.device = device;
    params.operation = operation;
    vk_unix_call(unix_vkGetDeferredOperationMaxConcurrencyKHR, &params);
    return params.result;
}

VkResult WINAPI vkGetDeferredOperationResultKHR(VkDevice device, VkDeferredOperationKHR operation)
{
    struct vkGetDeferredOperationResultKHR_params params;
    params.device = device;
    params.operation = operation;
    vk_unix_call(unix_vkGetDeferredOperationResultKHR, &params);
    return params.result;
}

void WINAPI vkGetDescriptorSetHostMappingVALVE(VkDevice device, VkDescriptorSet descriptorSet, void **ppData)
{
    struct vkGetDescriptorSetHostMappingVALVE_params params;
    params.device = device;
    params.descriptorSet = descriptorSet;
    params.ppData = ppData;
    vk_unix_call(unix_vkGetDescriptorSetHostMappingVALVE, &params);
}

void WINAPI vkGetDescriptorSetLayoutHostMappingInfoVALVE(VkDevice device, const VkDescriptorSetBindingReferenceVALVE *pBindingReference, VkDescriptorSetLayoutHostMappingInfoVALVE *pHostMapping)
{
    struct vkGetDescriptorSetLayoutHostMappingInfoVALVE_params params;
    params.device = device;
    params.pBindingReference = pBindingReference;
    params.pHostMapping = pHostMapping;
    vk_unix_call(unix_vkGetDescriptorSetLayoutHostMappingInfoVALVE, &params);
}

void WINAPI vkGetDescriptorSetLayoutSupport(VkDevice device, const VkDescriptorSetLayoutCreateInfo *pCreateInfo, VkDescriptorSetLayoutSupport *pSupport)
{
    struct vkGetDescriptorSetLayoutSupport_params params;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pSupport = pSupport;
    vk_unix_call(unix_vkGetDescriptorSetLayoutSupport, &params);
}

void WINAPI vkGetDescriptorSetLayoutSupportKHR(VkDevice device, const VkDescriptorSetLayoutCreateInfo *pCreateInfo, VkDescriptorSetLayoutSupport *pSupport)
{
    struct vkGetDescriptorSetLayoutSupportKHR_params params;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pSupport = pSupport;
    vk_unix_call(unix_vkGetDescriptorSetLayoutSupportKHR, &params);
}

void WINAPI vkGetDeviceAccelerationStructureCompatibilityKHR(VkDevice device, const VkAccelerationStructureVersionInfoKHR *pVersionInfo, VkAccelerationStructureCompatibilityKHR *pCompatibility)
{
    struct vkGetDeviceAccelerationStructureCompatibilityKHR_params params;
    params.device = device;
    params.pVersionInfo = pVersionInfo;
    params.pCompatibility = pCompatibility;
    vk_unix_call(unix_vkGetDeviceAccelerationStructureCompatibilityKHR, &params);
}

void WINAPI vkGetDeviceBufferMemoryRequirements(VkDevice device, const VkDeviceBufferMemoryRequirements *pInfo, VkMemoryRequirements2 *pMemoryRequirements)
{
    struct vkGetDeviceBufferMemoryRequirements_params params;
    params.device = device;
    params.pInfo = pInfo;
    params.pMemoryRequirements = pMemoryRequirements;
    vk_unix_call(unix_vkGetDeviceBufferMemoryRequirements, &params);
}

void WINAPI vkGetDeviceBufferMemoryRequirementsKHR(VkDevice device, const VkDeviceBufferMemoryRequirements *pInfo, VkMemoryRequirements2 *pMemoryRequirements)
{
    struct vkGetDeviceBufferMemoryRequirementsKHR_params params;
    params.device = device;
    params.pInfo = pInfo;
    params.pMemoryRequirements = pMemoryRequirements;
    vk_unix_call(unix_vkGetDeviceBufferMemoryRequirementsKHR, &params);
}

VkResult WINAPI vkGetDeviceFaultInfoEXT(VkDevice device, VkDeviceFaultCountsEXT *pFaultCounts, VkDeviceFaultInfoEXT *pFaultInfo)
{
    struct vkGetDeviceFaultInfoEXT_params params;
    params.device = device;
    params.pFaultCounts = pFaultCounts;
    params.pFaultInfo = pFaultInfo;
    vk_unix_call(unix_vkGetDeviceFaultInfoEXT, &params);
    return params.result;
}

void WINAPI vkGetDeviceGroupPeerMemoryFeatures(VkDevice device, uint32_t heapIndex, uint32_t localDeviceIndex, uint32_t remoteDeviceIndex, VkPeerMemoryFeatureFlags *pPeerMemoryFeatures)
{
    struct vkGetDeviceGroupPeerMemoryFeatures_params params;
    params.device = device;
    params.heapIndex = heapIndex;
    params.localDeviceIndex = localDeviceIndex;
    params.remoteDeviceIndex = remoteDeviceIndex;
    params.pPeerMemoryFeatures = pPeerMemoryFeatures;
    vk_unix_call(unix_vkGetDeviceGroupPeerMemoryFeatures, &params);
}

void WINAPI vkGetDeviceGroupPeerMemoryFeaturesKHR(VkDevice device, uint32_t heapIndex, uint32_t localDeviceIndex, uint32_t remoteDeviceIndex, VkPeerMemoryFeatureFlags *pPeerMemoryFeatures)
{
    struct vkGetDeviceGroupPeerMemoryFeaturesKHR_params params;
    params.device = device;
    params.heapIndex = heapIndex;
    params.localDeviceIndex = localDeviceIndex;
    params.remoteDeviceIndex = remoteDeviceIndex;
    params.pPeerMemoryFeatures = pPeerMemoryFeatures;
    vk_unix_call(unix_vkGetDeviceGroupPeerMemoryFeaturesKHR, &params);
}

VkResult WINAPI vkGetDeviceGroupPresentCapabilitiesKHR(VkDevice device, VkDeviceGroupPresentCapabilitiesKHR *pDeviceGroupPresentCapabilities)
{
    struct vkGetDeviceGroupPresentCapabilitiesKHR_params params;
    params.device = device;
    params.pDeviceGroupPresentCapabilities = pDeviceGroupPresentCapabilities;
    vk_unix_call(unix_vkGetDeviceGroupPresentCapabilitiesKHR, &params);
    return params.result;
}

VkResult WINAPI vkGetDeviceGroupSurfacePresentModesKHR(VkDevice device, VkSurfaceKHR surface, VkDeviceGroupPresentModeFlagsKHR *pModes)
{
    struct vkGetDeviceGroupSurfacePresentModesKHR_params params;
    params.device = device;
    params.surface = surface;
    params.pModes = pModes;
    vk_unix_call(unix_vkGetDeviceGroupSurfacePresentModesKHR, &params);
    return params.result;
}

void WINAPI vkGetDeviceImageMemoryRequirements(VkDevice device, const VkDeviceImageMemoryRequirements *pInfo, VkMemoryRequirements2 *pMemoryRequirements)
{
    struct vkGetDeviceImageMemoryRequirements_params params;
    params.device = device;
    params.pInfo = pInfo;
    params.pMemoryRequirements = pMemoryRequirements;
    vk_unix_call(unix_vkGetDeviceImageMemoryRequirements, &params);
}

void WINAPI vkGetDeviceImageMemoryRequirementsKHR(VkDevice device, const VkDeviceImageMemoryRequirements *pInfo, VkMemoryRequirements2 *pMemoryRequirements)
{
    struct vkGetDeviceImageMemoryRequirementsKHR_params params;
    params.device = device;
    params.pInfo = pInfo;
    params.pMemoryRequirements = pMemoryRequirements;
    vk_unix_call(unix_vkGetDeviceImageMemoryRequirementsKHR, &params);
}

void WINAPI vkGetDeviceImageSparseMemoryRequirements(VkDevice device, const VkDeviceImageMemoryRequirements *pInfo, uint32_t *pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements2 *pSparseMemoryRequirements)
{
    struct vkGetDeviceImageSparseMemoryRequirements_params params;
    params.device = device;
    params.pInfo = pInfo;
    params.pSparseMemoryRequirementCount = pSparseMemoryRequirementCount;
    params.pSparseMemoryRequirements = pSparseMemoryRequirements;
    vk_unix_call(unix_vkGetDeviceImageSparseMemoryRequirements, &params);
}

void WINAPI vkGetDeviceImageSparseMemoryRequirementsKHR(VkDevice device, const VkDeviceImageMemoryRequirements *pInfo, uint32_t *pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements2 *pSparseMemoryRequirements)
{
    struct vkGetDeviceImageSparseMemoryRequirementsKHR_params params;
    params.device = device;
    params.pInfo = pInfo;
    params.pSparseMemoryRequirementCount = pSparseMemoryRequirementCount;
    params.pSparseMemoryRequirements = pSparseMemoryRequirements;
    vk_unix_call(unix_vkGetDeviceImageSparseMemoryRequirementsKHR, &params);
}

void WINAPI vkGetDeviceMemoryCommitment(VkDevice device, VkDeviceMemory memory, VkDeviceSize *pCommittedMemoryInBytes)
{
    struct vkGetDeviceMemoryCommitment_params params;
    params.device = device;
    params.memory = memory;
    params.pCommittedMemoryInBytes = pCommittedMemoryInBytes;
    vk_unix_call(unix_vkGetDeviceMemoryCommitment, &params);
}

uint64_t WINAPI vkGetDeviceMemoryOpaqueCaptureAddress(VkDevice device, const VkDeviceMemoryOpaqueCaptureAddressInfo *pInfo)
{
    struct vkGetDeviceMemoryOpaqueCaptureAddress_params params;
    params.device = device;
    params.pInfo = pInfo;
    vk_unix_call(unix_vkGetDeviceMemoryOpaqueCaptureAddress, &params);
    return params.result;
}

uint64_t WINAPI vkGetDeviceMemoryOpaqueCaptureAddressKHR(VkDevice device, const VkDeviceMemoryOpaqueCaptureAddressInfo *pInfo)
{
    struct vkGetDeviceMemoryOpaqueCaptureAddressKHR_params params;
    params.device = device;
    params.pInfo = pInfo;
    vk_unix_call(unix_vkGetDeviceMemoryOpaqueCaptureAddressKHR, &params);
    return params.result;
}

void WINAPI vkGetDeviceMicromapCompatibilityEXT(VkDevice device, const VkMicromapVersionInfoEXT *pVersionInfo, VkAccelerationStructureCompatibilityKHR *pCompatibility)
{
    struct vkGetDeviceMicromapCompatibilityEXT_params params;
    params.device = device;
    params.pVersionInfo = pVersionInfo;
    params.pCompatibility = pCompatibility;
    vk_unix_call(unix_vkGetDeviceMicromapCompatibilityEXT, &params);
}

void WINAPI vkGetDeviceQueue(VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue *pQueue)
{
    struct vkGetDeviceQueue_params params;
    params.device = device;
    params.queueFamilyIndex = queueFamilyIndex;
    params.queueIndex = queueIndex;
    params.pQueue = pQueue;
    vk_unix_call(unix_vkGetDeviceQueue, &params);
}

void WINAPI vkGetDeviceQueue2(VkDevice device, const VkDeviceQueueInfo2 *pQueueInfo, VkQueue *pQueue)
{
    struct vkGetDeviceQueue2_params params;
    params.device = device;
    params.pQueueInfo = pQueueInfo;
    params.pQueue = pQueue;
    vk_unix_call(unix_vkGetDeviceQueue2, &params);
}

VkResult WINAPI vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI(VkDevice device, VkRenderPass renderpass, VkExtent2D *pMaxWorkgroupSize)
{
    struct vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI_params params;
    params.device = device;
    params.renderpass = renderpass;
    params.pMaxWorkgroupSize = pMaxWorkgroupSize;
    vk_unix_call(unix_vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI, &params);
    return params.result;
}

VkResult WINAPI vkGetDynamicRenderingTilePropertiesQCOM(VkDevice device, const VkRenderingInfo *pRenderingInfo, VkTilePropertiesQCOM *pProperties)
{
    struct vkGetDynamicRenderingTilePropertiesQCOM_params params;
    params.device = device;
    params.pRenderingInfo = pRenderingInfo;
    params.pProperties = pProperties;
    vk_unix_call(unix_vkGetDynamicRenderingTilePropertiesQCOM, &params);
    return params.result;
}

VkResult WINAPI vkGetEventStatus(VkDevice device, VkEvent event)
{
    struct vkGetEventStatus_params params;
    params.device = device;
    params.event = event;
    vk_unix_call(unix_vkGetEventStatus, &params);
    return params.result;
}

VkResult WINAPI vkGetFenceStatus(VkDevice device, VkFence fence)
{
    struct vkGetFenceStatus_params params;
    params.device = device;
    params.fence = fence;
    vk_unix_call(unix_vkGetFenceStatus, &params);
    return params.result;
}

VkResult WINAPI vkGetFramebufferTilePropertiesQCOM(VkDevice device, VkFramebuffer framebuffer, uint32_t *pPropertiesCount, VkTilePropertiesQCOM *pProperties)
{
    struct vkGetFramebufferTilePropertiesQCOM_params params;
    params.device = device;
    params.framebuffer = framebuffer;
    params.pPropertiesCount = pPropertiesCount;
    params.pProperties = pProperties;
    vk_unix_call(unix_vkGetFramebufferTilePropertiesQCOM, &params);
    return params.result;
}

void WINAPI vkGetGeneratedCommandsMemoryRequirementsNV(VkDevice device, const VkGeneratedCommandsMemoryRequirementsInfoNV *pInfo, VkMemoryRequirements2 *pMemoryRequirements)
{
    struct vkGetGeneratedCommandsMemoryRequirementsNV_params params;
    params.device = device;
    params.pInfo = pInfo;
    params.pMemoryRequirements = pMemoryRequirements;
    vk_unix_call(unix_vkGetGeneratedCommandsMemoryRequirementsNV, &params);
}

void WINAPI vkGetImageMemoryRequirements(VkDevice device, VkImage image, VkMemoryRequirements *pMemoryRequirements)
{
    struct vkGetImageMemoryRequirements_params params;
    params.device = device;
    params.image = image;
    params.pMemoryRequirements = pMemoryRequirements;
    vk_unix_call(unix_vkGetImageMemoryRequirements, &params);
}

void WINAPI vkGetImageMemoryRequirements2(VkDevice device, const VkImageMemoryRequirementsInfo2 *pInfo, VkMemoryRequirements2 *pMemoryRequirements)
{
    struct vkGetImageMemoryRequirements2_params params;
    params.device = device;
    params.pInfo = pInfo;
    params.pMemoryRequirements = pMemoryRequirements;
    vk_unix_call(unix_vkGetImageMemoryRequirements2, &params);
}

void WINAPI vkGetImageMemoryRequirements2KHR(VkDevice device, const VkImageMemoryRequirementsInfo2 *pInfo, VkMemoryRequirements2 *pMemoryRequirements)
{
    struct vkGetImageMemoryRequirements2KHR_params params;
    params.device = device;
    params.pInfo = pInfo;
    params.pMemoryRequirements = pMemoryRequirements;
    vk_unix_call(unix_vkGetImageMemoryRequirements2KHR, &params);
}

void WINAPI vkGetImageSparseMemoryRequirements(VkDevice device, VkImage image, uint32_t *pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements *pSparseMemoryRequirements)
{
    struct vkGetImageSparseMemoryRequirements_params params;
    params.device = device;
    params.image = image;
    params.pSparseMemoryRequirementCount = pSparseMemoryRequirementCount;
    params.pSparseMemoryRequirements = pSparseMemoryRequirements;
    vk_unix_call(unix_vkGetImageSparseMemoryRequirements, &params);
}

void WINAPI vkGetImageSparseMemoryRequirements2(VkDevice device, const VkImageSparseMemoryRequirementsInfo2 *pInfo, uint32_t *pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements2 *pSparseMemoryRequirements)
{
    struct vkGetImageSparseMemoryRequirements2_params params;
    params.device = device;
    params.pInfo = pInfo;
    params.pSparseMemoryRequirementCount = pSparseMemoryRequirementCount;
    params.pSparseMemoryRequirements = pSparseMemoryRequirements;
    vk_unix_call(unix_vkGetImageSparseMemoryRequirements2, &params);
}

void WINAPI vkGetImageSparseMemoryRequirements2KHR(VkDevice device, const VkImageSparseMemoryRequirementsInfo2 *pInfo, uint32_t *pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements2 *pSparseMemoryRequirements)
{
    struct vkGetImageSparseMemoryRequirements2KHR_params params;
    params.device = device;
    params.pInfo = pInfo;
    params.pSparseMemoryRequirementCount = pSparseMemoryRequirementCount;
    params.pSparseMemoryRequirements = pSparseMemoryRequirements;
    vk_unix_call(unix_vkGetImageSparseMemoryRequirements2KHR, &params);
}

void WINAPI vkGetImageSubresourceLayout(VkDevice device, VkImage image, const VkImageSubresource *pSubresource, VkSubresourceLayout *pLayout)
{
    struct vkGetImageSubresourceLayout_params params;
    params.device = device;
    params.image = image;
    params.pSubresource = pSubresource;
    params.pLayout = pLayout;
    vk_unix_call(unix_vkGetImageSubresourceLayout, &params);
}

void WINAPI vkGetImageSubresourceLayout2EXT(VkDevice device, VkImage image, const VkImageSubresource2EXT *pSubresource, VkSubresourceLayout2EXT *pLayout)
{
    struct vkGetImageSubresourceLayout2EXT_params params;
    params.device = device;
    params.image = image;
    params.pSubresource = pSubresource;
    params.pLayout = pLayout;
    vk_unix_call(unix_vkGetImageSubresourceLayout2EXT, &params);
}

VkResult WINAPI vkGetImageViewAddressNVX(VkDevice device, VkImageView imageView, VkImageViewAddressPropertiesNVX *pProperties)
{
    struct vkGetImageViewAddressNVX_params params;
    params.device = device;
    params.imageView = imageView;
    params.pProperties = pProperties;
    vk_unix_call(unix_vkGetImageViewAddressNVX, &params);
    return params.result;
}

uint32_t WINAPI vkGetImageViewHandleNVX(VkDevice device, const VkImageViewHandleInfoNVX *pInfo)
{
    struct vkGetImageViewHandleNVX_params params;
    params.device = device;
    params.pInfo = pInfo;
    vk_unix_call(unix_vkGetImageViewHandleNVX, &params);
    return params.result;
}

VkResult WINAPI vkGetMemoryHostPointerPropertiesEXT(VkDevice device, VkExternalMemoryHandleTypeFlagBits handleType, const void *pHostPointer, VkMemoryHostPointerPropertiesEXT *pMemoryHostPointerProperties)
{
    struct vkGetMemoryHostPointerPropertiesEXT_params params;
    params.device = device;
    params.handleType = handleType;
    params.pHostPointer = pHostPointer;
    params.pMemoryHostPointerProperties = pMemoryHostPointerProperties;
    vk_unix_call(unix_vkGetMemoryHostPointerPropertiesEXT, &params);
    return params.result;
}

void WINAPI vkGetMicromapBuildSizesEXT(VkDevice device, VkAccelerationStructureBuildTypeKHR buildType, const VkMicromapBuildInfoEXT *pBuildInfo, VkMicromapBuildSizesInfoEXT *pSizeInfo)
{
    struct vkGetMicromapBuildSizesEXT_params params;
    params.device = device;
    params.buildType = buildType;
    params.pBuildInfo = pBuildInfo;
    params.pSizeInfo = pSizeInfo;
    vk_unix_call(unix_vkGetMicromapBuildSizesEXT, &params);
}

VkResult WINAPI vkGetPerformanceParameterINTEL(VkDevice device, VkPerformanceParameterTypeINTEL parameter, VkPerformanceValueINTEL *pValue)
{
    struct vkGetPerformanceParameterINTEL_params params;
    params.device = device;
    params.parameter = parameter;
    params.pValue = pValue;
    vk_unix_call(unix_vkGetPerformanceParameterINTEL, &params);
    return params.result;
}

VkResult WINAPI vkGetPhysicalDeviceCalibrateableTimeDomainsEXT(VkPhysicalDevice physicalDevice, uint32_t *pTimeDomainCount, VkTimeDomainEXT *pTimeDomains)
{
    struct vkGetPhysicalDeviceCalibrateableTimeDomainsEXT_params params;
    params.physicalDevice = physicalDevice;
    params.pTimeDomainCount = pTimeDomainCount;
    params.pTimeDomains = pTimeDomains;
    vk_unix_call(unix_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT, &params);
    return params.result;
}

VkResult WINAPI vkGetPhysicalDeviceCooperativeMatrixPropertiesNV(VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount, VkCooperativeMatrixPropertiesNV *pProperties)
{
    struct vkGetPhysicalDeviceCooperativeMatrixPropertiesNV_params params;
    params.physicalDevice = physicalDevice;
    params.pPropertyCount = pPropertyCount;
    params.pProperties = pProperties;
    vk_unix_call(unix_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV, &params);
    return params.result;
}

void WINAPI vkGetPhysicalDeviceExternalBufferProperties(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalBufferInfo *pExternalBufferInfo, VkExternalBufferProperties *pExternalBufferProperties)
{
    struct vkGetPhysicalDeviceExternalBufferProperties_params params;
    params.physicalDevice = physicalDevice;
    params.pExternalBufferInfo = pExternalBufferInfo;
    params.pExternalBufferProperties = pExternalBufferProperties;
    vk_unix_call(unix_vkGetPhysicalDeviceExternalBufferProperties, &params);
}

void WINAPI vkGetPhysicalDeviceExternalBufferPropertiesKHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalBufferInfo *pExternalBufferInfo, VkExternalBufferProperties *pExternalBufferProperties)
{
    struct vkGetPhysicalDeviceExternalBufferPropertiesKHR_params params;
    params.physicalDevice = physicalDevice;
    params.pExternalBufferInfo = pExternalBufferInfo;
    params.pExternalBufferProperties = pExternalBufferProperties;
    vk_unix_call(unix_vkGetPhysicalDeviceExternalBufferPropertiesKHR, &params);
}

void WINAPI vkGetPhysicalDeviceExternalFenceProperties(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalFenceInfo *pExternalFenceInfo, VkExternalFenceProperties *pExternalFenceProperties)
{
    struct vkGetPhysicalDeviceExternalFenceProperties_params params;
    params.physicalDevice = physicalDevice;
    params.pExternalFenceInfo = pExternalFenceInfo;
    params.pExternalFenceProperties = pExternalFenceProperties;
    vk_unix_call(unix_vkGetPhysicalDeviceExternalFenceProperties, &params);
}

void WINAPI vkGetPhysicalDeviceExternalFencePropertiesKHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalFenceInfo *pExternalFenceInfo, VkExternalFenceProperties *pExternalFenceProperties)
{
    struct vkGetPhysicalDeviceExternalFencePropertiesKHR_params params;
    params.physicalDevice = physicalDevice;
    params.pExternalFenceInfo = pExternalFenceInfo;
    params.pExternalFenceProperties = pExternalFenceProperties;
    vk_unix_call(unix_vkGetPhysicalDeviceExternalFencePropertiesKHR, &params);
}

void WINAPI vkGetPhysicalDeviceExternalSemaphoreProperties(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalSemaphoreInfo *pExternalSemaphoreInfo, VkExternalSemaphoreProperties *pExternalSemaphoreProperties)
{
    struct vkGetPhysicalDeviceExternalSemaphoreProperties_params params;
    params.physicalDevice = physicalDevice;
    params.pExternalSemaphoreInfo = pExternalSemaphoreInfo;
    params.pExternalSemaphoreProperties = pExternalSemaphoreProperties;
    vk_unix_call(unix_vkGetPhysicalDeviceExternalSemaphoreProperties, &params);
}

void WINAPI vkGetPhysicalDeviceExternalSemaphorePropertiesKHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalSemaphoreInfo *pExternalSemaphoreInfo, VkExternalSemaphoreProperties *pExternalSemaphoreProperties)
{
    struct vkGetPhysicalDeviceExternalSemaphorePropertiesKHR_params params;
    params.physicalDevice = physicalDevice;
    params.pExternalSemaphoreInfo = pExternalSemaphoreInfo;
    params.pExternalSemaphoreProperties = pExternalSemaphoreProperties;
    vk_unix_call(unix_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR, &params);
}

void WINAPI vkGetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures *pFeatures)
{
    struct vkGetPhysicalDeviceFeatures_params params;
    params.physicalDevice = physicalDevice;
    params.pFeatures = pFeatures;
    vk_unix_call(unix_vkGetPhysicalDeviceFeatures, &params);
}

void WINAPI vkGetPhysicalDeviceFeatures2(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2 *pFeatures)
{
    struct vkGetPhysicalDeviceFeatures2_params params;
    params.physicalDevice = physicalDevice;
    params.pFeatures = pFeatures;
    vk_unix_call(unix_vkGetPhysicalDeviceFeatures2, &params);
}

void WINAPI vkGetPhysicalDeviceFeatures2KHR(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2 *pFeatures)
{
    struct vkGetPhysicalDeviceFeatures2KHR_params params;
    params.physicalDevice = physicalDevice;
    params.pFeatures = pFeatures;
    vk_unix_call(unix_vkGetPhysicalDeviceFeatures2KHR, &params);
}

void WINAPI vkGetPhysicalDeviceFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties *pFormatProperties)
{
    struct vkGetPhysicalDeviceFormatProperties_params params;
    params.physicalDevice = physicalDevice;
    params.format = format;
    params.pFormatProperties = pFormatProperties;
    vk_unix_call(unix_vkGetPhysicalDeviceFormatProperties, &params);
}

void WINAPI vkGetPhysicalDeviceFormatProperties2(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties2 *pFormatProperties)
{
    struct vkGetPhysicalDeviceFormatProperties2_params params;
    params.physicalDevice = physicalDevice;
    params.format = format;
    params.pFormatProperties = pFormatProperties;
    vk_unix_call(unix_vkGetPhysicalDeviceFormatProperties2, &params);
}

void WINAPI vkGetPhysicalDeviceFormatProperties2KHR(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties2 *pFormatProperties)
{
    struct vkGetPhysicalDeviceFormatProperties2KHR_params params;
    params.physicalDevice = physicalDevice;
    params.format = format;
    params.pFormatProperties = pFormatProperties;
    vk_unix_call(unix_vkGetPhysicalDeviceFormatProperties2KHR, &params);
}

VkResult WINAPI vkGetPhysicalDeviceFragmentShadingRatesKHR(VkPhysicalDevice physicalDevice, uint32_t *pFragmentShadingRateCount, VkPhysicalDeviceFragmentShadingRateKHR *pFragmentShadingRates)
{
    struct vkGetPhysicalDeviceFragmentShadingRatesKHR_params params;
    params.physicalDevice = physicalDevice;
    params.pFragmentShadingRateCount = pFragmentShadingRateCount;
    params.pFragmentShadingRates = pFragmentShadingRates;
    vk_unix_call(unix_vkGetPhysicalDeviceFragmentShadingRatesKHR, &params);
    return params.result;
}

VkResult WINAPI vkGetPhysicalDeviceImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, VkImageFormatProperties *pImageFormatProperties)
{
    struct vkGetPhysicalDeviceImageFormatProperties_params params;
    params.physicalDevice = physicalDevice;
    params.format = format;
    params.type = type;
    params.tiling = tiling;
    params.usage = usage;
    params.flags = flags;
    params.pImageFormatProperties = pImageFormatProperties;
    vk_unix_call(unix_vkGetPhysicalDeviceImageFormatProperties, &params);
    return params.result;
}

VkResult WINAPI vkGetPhysicalDeviceImageFormatProperties2(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceImageFormatInfo2 *pImageFormatInfo, VkImageFormatProperties2 *pImageFormatProperties)
{
    struct vkGetPhysicalDeviceImageFormatProperties2_params params;
    params.physicalDevice = physicalDevice;
    params.pImageFormatInfo = pImageFormatInfo;
    params.pImageFormatProperties = pImageFormatProperties;
    vk_unix_call(unix_vkGetPhysicalDeviceImageFormatProperties2, &params);
    return params.result;
}

VkResult WINAPI vkGetPhysicalDeviceImageFormatProperties2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceImageFormatInfo2 *pImageFormatInfo, VkImageFormatProperties2 *pImageFormatProperties)
{
    struct vkGetPhysicalDeviceImageFormatProperties2KHR_params params;
    params.physicalDevice = physicalDevice;
    params.pImageFormatInfo = pImageFormatInfo;
    params.pImageFormatProperties = pImageFormatProperties;
    vk_unix_call(unix_vkGetPhysicalDeviceImageFormatProperties2KHR, &params);
    return params.result;
}

void WINAPI vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties *pMemoryProperties)
{
    struct vkGetPhysicalDeviceMemoryProperties_params params;
    params.physicalDevice = physicalDevice;
    params.pMemoryProperties = pMemoryProperties;
    vk_unix_call(unix_vkGetPhysicalDeviceMemoryProperties, &params);
}

void WINAPI vkGetPhysicalDeviceMemoryProperties2(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties2 *pMemoryProperties)
{
    struct vkGetPhysicalDeviceMemoryProperties2_params params;
    params.physicalDevice = physicalDevice;
    params.pMemoryProperties = pMemoryProperties;
    vk_unix_call(unix_vkGetPhysicalDeviceMemoryProperties2, &params);
}

void WINAPI vkGetPhysicalDeviceMemoryProperties2KHR(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties2 *pMemoryProperties)
{
    struct vkGetPhysicalDeviceMemoryProperties2KHR_params params;
    params.physicalDevice = physicalDevice;
    params.pMemoryProperties = pMemoryProperties;
    vk_unix_call(unix_vkGetPhysicalDeviceMemoryProperties2KHR, &params);
}

void WINAPI vkGetPhysicalDeviceMultisamplePropertiesEXT(VkPhysicalDevice physicalDevice, VkSampleCountFlagBits samples, VkMultisamplePropertiesEXT *pMultisampleProperties)
{
    struct vkGetPhysicalDeviceMultisamplePropertiesEXT_params params;
    params.physicalDevice = physicalDevice;
    params.samples = samples;
    params.pMultisampleProperties = pMultisampleProperties;
    vk_unix_call(unix_vkGetPhysicalDeviceMultisamplePropertiesEXT, &params);
}

VkResult WINAPI vkGetPhysicalDeviceOpticalFlowImageFormatsNV(VkPhysicalDevice physicalDevice, const VkOpticalFlowImageFormatInfoNV *pOpticalFlowImageFormatInfo, uint32_t *pFormatCount, VkOpticalFlowImageFormatPropertiesNV *pImageFormatProperties)
{
    struct vkGetPhysicalDeviceOpticalFlowImageFormatsNV_params params;
    params.physicalDevice = physicalDevice;
    params.pOpticalFlowImageFormatInfo = pOpticalFlowImageFormatInfo;
    params.pFormatCount = pFormatCount;
    params.pImageFormatProperties = pImageFormatProperties;
    vk_unix_call(unix_vkGetPhysicalDeviceOpticalFlowImageFormatsNV, &params);
    return params.result;
}

VkResult WINAPI vkGetPhysicalDevicePresentRectanglesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t *pRectCount, VkRect2D *pRects)
{
    struct vkGetPhysicalDevicePresentRectanglesKHR_params params;
    params.physicalDevice = physicalDevice;
    params.surface = surface;
    params.pRectCount = pRectCount;
    params.pRects = pRects;
    vk_unix_call(unix_vkGetPhysicalDevicePresentRectanglesKHR, &params);
    return params.result;
}

void WINAPI vkGetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties *pProperties)
{
    struct vkGetPhysicalDeviceProperties_params params;
    params.physicalDevice = physicalDevice;
    params.pProperties = pProperties;
    vk_unix_call(unix_vkGetPhysicalDeviceProperties, &params);
}

void WINAPI vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR(VkPhysicalDevice physicalDevice, const VkQueryPoolPerformanceCreateInfoKHR *pPerformanceQueryCreateInfo, uint32_t *pNumPasses)
{
    struct vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR_params params;
    params.physicalDevice = physicalDevice;
    params.pPerformanceQueryCreateInfo = pPerformanceQueryCreateInfo;
    params.pNumPasses = pNumPasses;
    vk_unix_call(unix_vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR, &params);
}

void WINAPI vkGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice, uint32_t *pQueueFamilyPropertyCount, VkQueueFamilyProperties *pQueueFamilyProperties)
{
    struct vkGetPhysicalDeviceQueueFamilyProperties_params params;
    params.physicalDevice = physicalDevice;
    params.pQueueFamilyPropertyCount = pQueueFamilyPropertyCount;
    params.pQueueFamilyProperties = pQueueFamilyProperties;
    vk_unix_call(unix_vkGetPhysicalDeviceQueueFamilyProperties, &params);
}

void WINAPI vkGetPhysicalDeviceQueueFamilyProperties2(VkPhysicalDevice physicalDevice, uint32_t *pQueueFamilyPropertyCount, VkQueueFamilyProperties2 *pQueueFamilyProperties)
{
    struct vkGetPhysicalDeviceQueueFamilyProperties2_params params;
    params.physicalDevice = physicalDevice;
    params.pQueueFamilyPropertyCount = pQueueFamilyPropertyCount;
    params.pQueueFamilyProperties = pQueueFamilyProperties;
    vk_unix_call(unix_vkGetPhysicalDeviceQueueFamilyProperties2, &params);
}

void WINAPI vkGetPhysicalDeviceQueueFamilyProperties2KHR(VkPhysicalDevice physicalDevice, uint32_t *pQueueFamilyPropertyCount, VkQueueFamilyProperties2 *pQueueFamilyProperties)
{
    struct vkGetPhysicalDeviceQueueFamilyProperties2KHR_params params;
    params.physicalDevice = physicalDevice;
    params.pQueueFamilyPropertyCount = pQueueFamilyPropertyCount;
    params.pQueueFamilyProperties = pQueueFamilyProperties;
    vk_unix_call(unix_vkGetPhysicalDeviceQueueFamilyProperties2KHR, &params);
}

void WINAPI vkGetPhysicalDeviceSparseImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkSampleCountFlagBits samples, VkImageUsageFlags usage, VkImageTiling tiling, uint32_t *pPropertyCount, VkSparseImageFormatProperties *pProperties)
{
    struct vkGetPhysicalDeviceSparseImageFormatProperties_params params;
    params.physicalDevice = physicalDevice;
    params.format = format;
    params.type = type;
    params.samples = samples;
    params.usage = usage;
    params.tiling = tiling;
    params.pPropertyCount = pPropertyCount;
    params.pProperties = pProperties;
    vk_unix_call(unix_vkGetPhysicalDeviceSparseImageFormatProperties, &params);
}

void WINAPI vkGetPhysicalDeviceSparseImageFormatProperties2(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSparseImageFormatInfo2 *pFormatInfo, uint32_t *pPropertyCount, VkSparseImageFormatProperties2 *pProperties)
{
    struct vkGetPhysicalDeviceSparseImageFormatProperties2_params params;
    params.physicalDevice = physicalDevice;
    params.pFormatInfo = pFormatInfo;
    params.pPropertyCount = pPropertyCount;
    params.pProperties = pProperties;
    vk_unix_call(unix_vkGetPhysicalDeviceSparseImageFormatProperties2, &params);
}

void WINAPI vkGetPhysicalDeviceSparseImageFormatProperties2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSparseImageFormatInfo2 *pFormatInfo, uint32_t *pPropertyCount, VkSparseImageFormatProperties2 *pProperties)
{
    struct vkGetPhysicalDeviceSparseImageFormatProperties2KHR_params params;
    params.physicalDevice = physicalDevice;
    params.pFormatInfo = pFormatInfo;
    params.pPropertyCount = pPropertyCount;
    params.pProperties = pProperties;
    vk_unix_call(unix_vkGetPhysicalDeviceSparseImageFormatProperties2KHR, &params);
}

VkResult WINAPI vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV(VkPhysicalDevice physicalDevice, uint32_t *pCombinationCount, VkFramebufferMixedSamplesCombinationNV *pCombinations)
{
    struct vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV_params params;
    params.physicalDevice = physicalDevice;
    params.pCombinationCount = pCombinationCount;
    params.pCombinations = pCombinations;
    vk_unix_call(unix_vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV, &params);
    return params.result;
}

VkResult WINAPI vkGetPhysicalDeviceSurfaceCapabilities2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR *pSurfaceInfo, VkSurfaceCapabilities2KHR *pSurfaceCapabilities)
{
    struct vkGetPhysicalDeviceSurfaceCapabilities2KHR_params params;
    params.physicalDevice = physicalDevice;
    params.pSurfaceInfo = pSurfaceInfo;
    params.pSurfaceCapabilities = pSurfaceCapabilities;
    vk_unix_call(unix_vkGetPhysicalDeviceSurfaceCapabilities2KHR, &params);
    return params.result;
}

VkResult WINAPI vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR *pSurfaceCapabilities)
{
    struct vkGetPhysicalDeviceSurfaceCapabilitiesKHR_params params;
    params.physicalDevice = physicalDevice;
    params.surface = surface;
    params.pSurfaceCapabilities = pSurfaceCapabilities;
    vk_unix_call(unix_vkGetPhysicalDeviceSurfaceCapabilitiesKHR, &params);
    return params.result;
}

VkResult WINAPI vkGetPhysicalDeviceSurfaceFormats2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR *pSurfaceInfo, uint32_t *pSurfaceFormatCount, VkSurfaceFormat2KHR *pSurfaceFormats)
{
    struct vkGetPhysicalDeviceSurfaceFormats2KHR_params params;
    params.physicalDevice = physicalDevice;
    params.pSurfaceInfo = pSurfaceInfo;
    params.pSurfaceFormatCount = pSurfaceFormatCount;
    params.pSurfaceFormats = pSurfaceFormats;
    vk_unix_call(unix_vkGetPhysicalDeviceSurfaceFormats2KHR, &params);
    return params.result;
}

VkResult WINAPI vkGetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t *pSurfaceFormatCount, VkSurfaceFormatKHR *pSurfaceFormats)
{
    struct vkGetPhysicalDeviceSurfaceFormatsKHR_params params;
    params.physicalDevice = physicalDevice;
    params.surface = surface;
    params.pSurfaceFormatCount = pSurfaceFormatCount;
    params.pSurfaceFormats = pSurfaceFormats;
    vk_unix_call(unix_vkGetPhysicalDeviceSurfaceFormatsKHR, &params);
    return params.result;
}

VkResult WINAPI vkGetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t *pPresentModeCount, VkPresentModeKHR *pPresentModes)
{
    struct vkGetPhysicalDeviceSurfacePresentModesKHR_params params;
    params.physicalDevice = physicalDevice;
    params.surface = surface;
    params.pPresentModeCount = pPresentModeCount;
    params.pPresentModes = pPresentModes;
    vk_unix_call(unix_vkGetPhysicalDeviceSurfacePresentModesKHR, &params);
    return params.result;
}

VkResult WINAPI vkGetPhysicalDeviceSurfaceSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, VkSurfaceKHR surface, VkBool32 *pSupported)
{
    struct vkGetPhysicalDeviceSurfaceSupportKHR_params params;
    params.physicalDevice = physicalDevice;
    params.queueFamilyIndex = queueFamilyIndex;
    params.surface = surface;
    params.pSupported = pSupported;
    vk_unix_call(unix_vkGetPhysicalDeviceSurfaceSupportKHR, &params);
    return params.result;
}

VkResult WINAPI vkGetPhysicalDeviceToolProperties(VkPhysicalDevice physicalDevice, uint32_t *pToolCount, VkPhysicalDeviceToolProperties *pToolProperties)
{
    struct vkGetPhysicalDeviceToolProperties_params params;
    params.physicalDevice = physicalDevice;
    params.pToolCount = pToolCount;
    params.pToolProperties = pToolProperties;
    vk_unix_call(unix_vkGetPhysicalDeviceToolProperties, &params);
    return params.result;
}

VkResult WINAPI vkGetPhysicalDeviceToolPropertiesEXT(VkPhysicalDevice physicalDevice, uint32_t *pToolCount, VkPhysicalDeviceToolProperties *pToolProperties)
{
    struct vkGetPhysicalDeviceToolPropertiesEXT_params params;
    params.physicalDevice = physicalDevice;
    params.pToolCount = pToolCount;
    params.pToolProperties = pToolProperties;
    vk_unix_call(unix_vkGetPhysicalDeviceToolPropertiesEXT, &params);
    return params.result;
}

VkBool32 WINAPI vkGetPhysicalDeviceWin32PresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex)
{
    struct vkGetPhysicalDeviceWin32PresentationSupportKHR_params params;
    params.physicalDevice = physicalDevice;
    params.queueFamilyIndex = queueFamilyIndex;
    vk_unix_call(unix_vkGetPhysicalDeviceWin32PresentationSupportKHR, &params);
    return params.result;
}

VkResult WINAPI vkGetPipelineCacheData(VkDevice device, VkPipelineCache pipelineCache, size_t *pDataSize, void *pData)
{
    struct vkGetPipelineCacheData_params params;
    params.device = device;
    params.pipelineCache = pipelineCache;
    params.pDataSize = pDataSize;
    params.pData = pData;
    vk_unix_call(unix_vkGetPipelineCacheData, &params);
    return params.result;
}

VkResult WINAPI vkGetPipelineExecutableInternalRepresentationsKHR(VkDevice device, const VkPipelineExecutableInfoKHR *pExecutableInfo, uint32_t *pInternalRepresentationCount, VkPipelineExecutableInternalRepresentationKHR *pInternalRepresentations)
{
    struct vkGetPipelineExecutableInternalRepresentationsKHR_params params;
    params.device = device;
    params.pExecutableInfo = pExecutableInfo;
    params.pInternalRepresentationCount = pInternalRepresentationCount;
    params.pInternalRepresentations = pInternalRepresentations;
    vk_unix_call(unix_vkGetPipelineExecutableInternalRepresentationsKHR, &params);
    return params.result;
}

VkResult WINAPI vkGetPipelineExecutablePropertiesKHR(VkDevice device, const VkPipelineInfoKHR *pPipelineInfo, uint32_t *pExecutableCount, VkPipelineExecutablePropertiesKHR *pProperties)
{
    struct vkGetPipelineExecutablePropertiesKHR_params params;
    params.device = device;
    params.pPipelineInfo = pPipelineInfo;
    params.pExecutableCount = pExecutableCount;
    params.pProperties = pProperties;
    vk_unix_call(unix_vkGetPipelineExecutablePropertiesKHR, &params);
    return params.result;
}

VkResult WINAPI vkGetPipelineExecutableStatisticsKHR(VkDevice device, const VkPipelineExecutableInfoKHR *pExecutableInfo, uint32_t *pStatisticCount, VkPipelineExecutableStatisticKHR *pStatistics)
{
    struct vkGetPipelineExecutableStatisticsKHR_params params;
    params.device = device;
    params.pExecutableInfo = pExecutableInfo;
    params.pStatisticCount = pStatisticCount;
    params.pStatistics = pStatistics;
    vk_unix_call(unix_vkGetPipelineExecutableStatisticsKHR, &params);
    return params.result;
}

VkResult WINAPI vkGetPipelinePropertiesEXT(VkDevice device, const VkPipelineInfoEXT *pPipelineInfo, VkBaseOutStructure *pPipelineProperties)
{
    struct vkGetPipelinePropertiesEXT_params params;
    params.device = device;
    params.pPipelineInfo = pPipelineInfo;
    params.pPipelineProperties = pPipelineProperties;
    vk_unix_call(unix_vkGetPipelinePropertiesEXT, &params);
    return params.result;
}

void WINAPI vkGetPrivateData(VkDevice device, VkObjectType objectType, uint64_t objectHandle, VkPrivateDataSlot privateDataSlot, uint64_t *pData)
{
    struct vkGetPrivateData_params params;
    params.device = device;
    params.objectType = objectType;
    params.objectHandle = objectHandle;
    params.privateDataSlot = privateDataSlot;
    params.pData = pData;
    vk_unix_call(unix_vkGetPrivateData, &params);
}

void WINAPI vkGetPrivateDataEXT(VkDevice device, VkObjectType objectType, uint64_t objectHandle, VkPrivateDataSlot privateDataSlot, uint64_t *pData)
{
    struct vkGetPrivateDataEXT_params params;
    params.device = device;
    params.objectType = objectType;
    params.objectHandle = objectHandle;
    params.privateDataSlot = privateDataSlot;
    params.pData = pData;
    vk_unix_call(unix_vkGetPrivateDataEXT, &params);
}

VkResult WINAPI vkGetQueryPoolResults(VkDevice device, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, size_t dataSize, void *pData, VkDeviceSize stride, VkQueryResultFlags flags)
{
    struct vkGetQueryPoolResults_params params;
    params.device = device;
    params.queryPool = queryPool;
    params.firstQuery = firstQuery;
    params.queryCount = queryCount;
    params.dataSize = dataSize;
    params.pData = pData;
    params.stride = stride;
    params.flags = flags;
    vk_unix_call(unix_vkGetQueryPoolResults, &params);
    return params.result;
}

void WINAPI vkGetQueueCheckpointData2NV(VkQueue queue, uint32_t *pCheckpointDataCount, VkCheckpointData2NV *pCheckpointData)
{
    struct vkGetQueueCheckpointData2NV_params params;
    params.queue = queue;
    params.pCheckpointDataCount = pCheckpointDataCount;
    params.pCheckpointData = pCheckpointData;
    vk_unix_call(unix_vkGetQueueCheckpointData2NV, &params);
}

void WINAPI vkGetQueueCheckpointDataNV(VkQueue queue, uint32_t *pCheckpointDataCount, VkCheckpointDataNV *pCheckpointData)
{
    struct vkGetQueueCheckpointDataNV_params params;
    params.queue = queue;
    params.pCheckpointDataCount = pCheckpointDataCount;
    params.pCheckpointData = pCheckpointData;
    vk_unix_call(unix_vkGetQueueCheckpointDataNV, &params);
}

VkResult WINAPI vkGetRayTracingCaptureReplayShaderGroupHandlesKHR(VkDevice device, VkPipeline pipeline, uint32_t firstGroup, uint32_t groupCount, size_t dataSize, void *pData)
{
    struct vkGetRayTracingCaptureReplayShaderGroupHandlesKHR_params params;
    params.device = device;
    params.pipeline = pipeline;
    params.firstGroup = firstGroup;
    params.groupCount = groupCount;
    params.dataSize = dataSize;
    params.pData = pData;
    vk_unix_call(unix_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR, &params);
    return params.result;
}

VkResult WINAPI vkGetRayTracingShaderGroupHandlesKHR(VkDevice device, VkPipeline pipeline, uint32_t firstGroup, uint32_t groupCount, size_t dataSize, void *pData)
{
    struct vkGetRayTracingShaderGroupHandlesKHR_params params;
    params.device = device;
    params.pipeline = pipeline;
    params.firstGroup = firstGroup;
    params.groupCount = groupCount;
    params.dataSize = dataSize;
    params.pData = pData;
    vk_unix_call(unix_vkGetRayTracingShaderGroupHandlesKHR, &params);
    return params.result;
}

VkResult WINAPI vkGetRayTracingShaderGroupHandlesNV(VkDevice device, VkPipeline pipeline, uint32_t firstGroup, uint32_t groupCount, size_t dataSize, void *pData)
{
    struct vkGetRayTracingShaderGroupHandlesNV_params params;
    params.device = device;
    params.pipeline = pipeline;
    params.firstGroup = firstGroup;
    params.groupCount = groupCount;
    params.dataSize = dataSize;
    params.pData = pData;
    vk_unix_call(unix_vkGetRayTracingShaderGroupHandlesNV, &params);
    return params.result;
}

VkDeviceSize WINAPI vkGetRayTracingShaderGroupStackSizeKHR(VkDevice device, VkPipeline pipeline, uint32_t group, VkShaderGroupShaderKHR groupShader)
{
    struct vkGetRayTracingShaderGroupStackSizeKHR_params params;
    params.device = device;
    params.pipeline = pipeline;
    params.group = group;
    params.groupShader = groupShader;
    vk_unix_call(unix_vkGetRayTracingShaderGroupStackSizeKHR, &params);
    return params.result;
}

void WINAPI vkGetRenderAreaGranularity(VkDevice device, VkRenderPass renderPass, VkExtent2D *pGranularity)
{
    struct vkGetRenderAreaGranularity_params params;
    params.device = device;
    params.renderPass = renderPass;
    params.pGranularity = pGranularity;
    vk_unix_call(unix_vkGetRenderAreaGranularity, &params);
}

VkResult WINAPI vkGetSemaphoreCounterValue(VkDevice device, VkSemaphore semaphore, uint64_t *pValue)
{
    struct vkGetSemaphoreCounterValue_params params;
    params.device = device;
    params.semaphore = semaphore;
    params.pValue = pValue;
    vk_unix_call(unix_vkGetSemaphoreCounterValue, &params);
    return params.result;
}

VkResult WINAPI vkGetSemaphoreCounterValueKHR(VkDevice device, VkSemaphore semaphore, uint64_t *pValue)
{
    struct vkGetSemaphoreCounterValueKHR_params params;
    params.device = device;
    params.semaphore = semaphore;
    params.pValue = pValue;
    vk_unix_call(unix_vkGetSemaphoreCounterValueKHR, &params);
    return params.result;
}

VkResult WINAPI vkGetShaderInfoAMD(VkDevice device, VkPipeline pipeline, VkShaderStageFlagBits shaderStage, VkShaderInfoTypeAMD infoType, size_t *pInfoSize, void *pInfo)
{
    struct vkGetShaderInfoAMD_params params;
    params.device = device;
    params.pipeline = pipeline;
    params.shaderStage = shaderStage;
    params.infoType = infoType;
    params.pInfoSize = pInfoSize;
    params.pInfo = pInfo;
    vk_unix_call(unix_vkGetShaderInfoAMD, &params);
    return params.result;
}

void WINAPI vkGetShaderModuleCreateInfoIdentifierEXT(VkDevice device, const VkShaderModuleCreateInfo *pCreateInfo, VkShaderModuleIdentifierEXT *pIdentifier)
{
    struct vkGetShaderModuleCreateInfoIdentifierEXT_params params;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pIdentifier = pIdentifier;
    vk_unix_call(unix_vkGetShaderModuleCreateInfoIdentifierEXT, &params);
}

void WINAPI vkGetShaderModuleIdentifierEXT(VkDevice device, VkShaderModule shaderModule, VkShaderModuleIdentifierEXT *pIdentifier)
{
    struct vkGetShaderModuleIdentifierEXT_params params;
    params.device = device;
    params.shaderModule = shaderModule;
    params.pIdentifier = pIdentifier;
    vk_unix_call(unix_vkGetShaderModuleIdentifierEXT, &params);
}

VkResult WINAPI vkGetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t *pSwapchainImageCount, VkImage *pSwapchainImages)
{
    struct vkGetSwapchainImagesKHR_params params;
    params.device = device;
    params.swapchain = swapchain;
    params.pSwapchainImageCount = pSwapchainImageCount;
    params.pSwapchainImages = pSwapchainImages;
    vk_unix_call(unix_vkGetSwapchainImagesKHR, &params);
    return params.result;
}

VkResult WINAPI vkGetValidationCacheDataEXT(VkDevice device, VkValidationCacheEXT validationCache, size_t *pDataSize, void *pData)
{
    struct vkGetValidationCacheDataEXT_params params;
    params.device = device;
    params.validationCache = validationCache;
    params.pDataSize = pDataSize;
    params.pData = pData;
    vk_unix_call(unix_vkGetValidationCacheDataEXT, &params);
    return params.result;
}

VkResult WINAPI vkInitializePerformanceApiINTEL(VkDevice device, const VkInitializePerformanceApiInfoINTEL *pInitializeInfo)
{
    struct vkInitializePerformanceApiINTEL_params params;
    params.device = device;
    params.pInitializeInfo = pInitializeInfo;
    vk_unix_call(unix_vkInitializePerformanceApiINTEL, &params);
    return params.result;
}

VkResult WINAPI vkInvalidateMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount, const VkMappedMemoryRange *pMemoryRanges)
{
    struct vkInvalidateMappedMemoryRanges_params params;
    params.device = device;
    params.memoryRangeCount = memoryRangeCount;
    params.pMemoryRanges = pMemoryRanges;
    vk_unix_call(unix_vkInvalidateMappedMemoryRanges, &params);
    return params.result;
}

VkResult WINAPI vkMapMemory(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void **ppData)
{
    struct vkMapMemory_params params;
    params.device = device;
    params.memory = memory;
    params.offset = offset;
    params.size = size;
    params.flags = flags;
    params.ppData = ppData;
    vk_unix_call(unix_vkMapMemory, &params);
    return params.result;
}

VkResult WINAPI vkMergePipelineCaches(VkDevice device, VkPipelineCache dstCache, uint32_t srcCacheCount, const VkPipelineCache *pSrcCaches)
{
    struct vkMergePipelineCaches_params params;
    params.device = device;
    params.dstCache = dstCache;
    params.srcCacheCount = srcCacheCount;
    params.pSrcCaches = pSrcCaches;
    vk_unix_call(unix_vkMergePipelineCaches, &params);
    return params.result;
}

VkResult WINAPI vkMergeValidationCachesEXT(VkDevice device, VkValidationCacheEXT dstCache, uint32_t srcCacheCount, const VkValidationCacheEXT *pSrcCaches)
{
    struct vkMergeValidationCachesEXT_params params;
    params.device = device;
    params.dstCache = dstCache;
    params.srcCacheCount = srcCacheCount;
    params.pSrcCaches = pSrcCaches;
    vk_unix_call(unix_vkMergeValidationCachesEXT, &params);
    return params.result;
}

void WINAPI vkQueueBeginDebugUtilsLabelEXT(VkQueue queue, const VkDebugUtilsLabelEXT *pLabelInfo)
{
    struct vkQueueBeginDebugUtilsLabelEXT_params params;
    params.queue = queue;
    params.pLabelInfo = pLabelInfo;
    vk_unix_call(unix_vkQueueBeginDebugUtilsLabelEXT, &params);
}

VkResult WINAPI vkQueueBindSparse(VkQueue queue, uint32_t bindInfoCount, const VkBindSparseInfo *pBindInfo, VkFence fence)
{
    struct vkQueueBindSparse_params params;
    params.queue = queue;
    params.bindInfoCount = bindInfoCount;
    params.pBindInfo = pBindInfo;
    params.fence = fence;
    vk_unix_call(unix_vkQueueBindSparse, &params);
    return params.result;
}

void WINAPI vkQueueEndDebugUtilsLabelEXT(VkQueue queue)
{
    struct vkQueueEndDebugUtilsLabelEXT_params params;
    params.queue = queue;
    vk_unix_call(unix_vkQueueEndDebugUtilsLabelEXT, &params);
}

void WINAPI vkQueueInsertDebugUtilsLabelEXT(VkQueue queue, const VkDebugUtilsLabelEXT *pLabelInfo)
{
    struct vkQueueInsertDebugUtilsLabelEXT_params params;
    params.queue = queue;
    params.pLabelInfo = pLabelInfo;
    vk_unix_call(unix_vkQueueInsertDebugUtilsLabelEXT, &params);
}

VkResult WINAPI vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR *pPresentInfo)
{
    struct vkQueuePresentKHR_params params;
    params.queue = queue;
    params.pPresentInfo = pPresentInfo;
    vk_unix_call(unix_vkQueuePresentKHR, &params);
    return params.result;
}

VkResult WINAPI vkQueueSetPerformanceConfigurationINTEL(VkQueue queue, VkPerformanceConfigurationINTEL configuration)
{
    struct vkQueueSetPerformanceConfigurationINTEL_params params;
    params.queue = queue;
    params.configuration = configuration;
    vk_unix_call(unix_vkQueueSetPerformanceConfigurationINTEL, &params);
    return params.result;
}

VkResult WINAPI vkQueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo *pSubmits, VkFence fence)
{
    struct vkQueueSubmit_params params;
    params.queue = queue;
    params.submitCount = submitCount;
    params.pSubmits = pSubmits;
    params.fence = fence;
    vk_unix_call(unix_vkQueueSubmit, &params);
    return params.result;
}

VkResult WINAPI vkQueueSubmit2(VkQueue queue, uint32_t submitCount, const VkSubmitInfo2 *pSubmits, VkFence fence)
{
    struct vkQueueSubmit2_params params;
    params.queue = queue;
    params.submitCount = submitCount;
    params.pSubmits = pSubmits;
    params.fence = fence;
    vk_unix_call(unix_vkQueueSubmit2, &params);
    return params.result;
}

VkResult WINAPI vkQueueSubmit2KHR(VkQueue queue, uint32_t submitCount, const VkSubmitInfo2 *pSubmits, VkFence fence)
{
    struct vkQueueSubmit2KHR_params params;
    params.queue = queue;
    params.submitCount = submitCount;
    params.pSubmits = pSubmits;
    params.fence = fence;
    vk_unix_call(unix_vkQueueSubmit2KHR, &params);
    return params.result;
}

VkResult WINAPI vkQueueWaitIdle(VkQueue queue)
{
    struct vkQueueWaitIdle_params params;
    params.queue = queue;
    vk_unix_call(unix_vkQueueWaitIdle, &params);
    return params.result;
}

VkResult WINAPI vkReleasePerformanceConfigurationINTEL(VkDevice device, VkPerformanceConfigurationINTEL configuration)
{
    struct vkReleasePerformanceConfigurationINTEL_params params;
    params.device = device;
    params.configuration = configuration;
    vk_unix_call(unix_vkReleasePerformanceConfigurationINTEL, &params);
    return params.result;
}

void WINAPI vkReleaseProfilingLockKHR(VkDevice device)
{
    struct vkReleaseProfilingLockKHR_params params;
    params.device = device;
    vk_unix_call(unix_vkReleaseProfilingLockKHR, &params);
}

VkResult WINAPI vkResetCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags)
{
    struct vkResetCommandBuffer_params params;
    params.commandBuffer = commandBuffer;
    params.flags = flags;
    vk_unix_call(unix_vkResetCommandBuffer, &params);
    return params.result;
}

VkResult WINAPI vkResetCommandPool(VkDevice device, VkCommandPool commandPool, VkCommandPoolResetFlags flags)
{
    struct vkResetCommandPool_params params;
    params.device = device;
    params.commandPool = commandPool;
    params.flags = flags;
    vk_unix_call(unix_vkResetCommandPool, &params);
    return params.result;
}

VkResult WINAPI vkResetDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorPoolResetFlags flags)
{
    struct vkResetDescriptorPool_params params;
    params.device = device;
    params.descriptorPool = descriptorPool;
    params.flags = flags;
    vk_unix_call(unix_vkResetDescriptorPool, &params);
    return params.result;
}

VkResult WINAPI vkResetEvent(VkDevice device, VkEvent event)
{
    struct vkResetEvent_params params;
    params.device = device;
    params.event = event;
    vk_unix_call(unix_vkResetEvent, &params);
    return params.result;
}

VkResult WINAPI vkResetFences(VkDevice device, uint32_t fenceCount, const VkFence *pFences)
{
    struct vkResetFences_params params;
    params.device = device;
    params.fenceCount = fenceCount;
    params.pFences = pFences;
    vk_unix_call(unix_vkResetFences, &params);
    return params.result;
}

void WINAPI vkResetQueryPool(VkDevice device, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount)
{
    struct vkResetQueryPool_params params;
    params.device = device;
    params.queryPool = queryPool;
    params.firstQuery = firstQuery;
    params.queryCount = queryCount;
    vk_unix_call(unix_vkResetQueryPool, &params);
}

void WINAPI vkResetQueryPoolEXT(VkDevice device, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount)
{
    struct vkResetQueryPoolEXT_params params;
    params.device = device;
    params.queryPool = queryPool;
    params.firstQuery = firstQuery;
    params.queryCount = queryCount;
    vk_unix_call(unix_vkResetQueryPoolEXT, &params);
}

VkResult WINAPI vkSetDebugUtilsObjectNameEXT(VkDevice device, const VkDebugUtilsObjectNameInfoEXT *pNameInfo)
{
    struct vkSetDebugUtilsObjectNameEXT_params params;
    params.device = device;
    params.pNameInfo = pNameInfo;
    vk_unix_call(unix_vkSetDebugUtilsObjectNameEXT, &params);
    return params.result;
}

VkResult WINAPI vkSetDebugUtilsObjectTagEXT(VkDevice device, const VkDebugUtilsObjectTagInfoEXT *pTagInfo)
{
    struct vkSetDebugUtilsObjectTagEXT_params params;
    params.device = device;
    params.pTagInfo = pTagInfo;
    vk_unix_call(unix_vkSetDebugUtilsObjectTagEXT, &params);
    return params.result;
}

void WINAPI vkSetDeviceMemoryPriorityEXT(VkDevice device, VkDeviceMemory memory, float priority)
{
    struct vkSetDeviceMemoryPriorityEXT_params params;
    params.device = device;
    params.memory = memory;
    params.priority = priority;
    vk_unix_call(unix_vkSetDeviceMemoryPriorityEXT, &params);
}

VkResult WINAPI vkSetEvent(VkDevice device, VkEvent event)
{
    struct vkSetEvent_params params;
    params.device = device;
    params.event = event;
    vk_unix_call(unix_vkSetEvent, &params);
    return params.result;
}

VkResult WINAPI vkSetPrivateData(VkDevice device, VkObjectType objectType, uint64_t objectHandle, VkPrivateDataSlot privateDataSlot, uint64_t data)
{
    struct vkSetPrivateData_params params;
    params.device = device;
    params.objectType = objectType;
    params.objectHandle = objectHandle;
    params.privateDataSlot = privateDataSlot;
    params.data = data;
    vk_unix_call(unix_vkSetPrivateData, &params);
    return params.result;
}

VkResult WINAPI vkSetPrivateDataEXT(VkDevice device, VkObjectType objectType, uint64_t objectHandle, VkPrivateDataSlot privateDataSlot, uint64_t data)
{
    struct vkSetPrivateDataEXT_params params;
    params.device = device;
    params.objectType = objectType;
    params.objectHandle = objectHandle;
    params.privateDataSlot = privateDataSlot;
    params.data = data;
    vk_unix_call(unix_vkSetPrivateDataEXT, &params);
    return params.result;
}

VkResult WINAPI vkSignalSemaphore(VkDevice device, const VkSemaphoreSignalInfo *pSignalInfo)
{
    struct vkSignalSemaphore_params params;
    params.device = device;
    params.pSignalInfo = pSignalInfo;
    vk_unix_call(unix_vkSignalSemaphore, &params);
    return params.result;
}

VkResult WINAPI vkSignalSemaphoreKHR(VkDevice device, const VkSemaphoreSignalInfo *pSignalInfo)
{
    struct vkSignalSemaphoreKHR_params params;
    params.device = device;
    params.pSignalInfo = pSignalInfo;
    vk_unix_call(unix_vkSignalSemaphoreKHR, &params);
    return params.result;
}

void WINAPI vkSubmitDebugUtilsMessageEXT(VkInstance instance, VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData)
{
    struct vkSubmitDebugUtilsMessageEXT_params params;
    params.instance = instance;
    params.messageSeverity = messageSeverity;
    params.messageTypes = messageTypes;
    params.pCallbackData = pCallbackData;
    vk_unix_call(unix_vkSubmitDebugUtilsMessageEXT, &params);
}

void WINAPI vkTrimCommandPool(VkDevice device, VkCommandPool commandPool, VkCommandPoolTrimFlags flags)
{
    struct vkTrimCommandPool_params params;
    params.device = device;
    params.commandPool = commandPool;
    params.flags = flags;
    vk_unix_call(unix_vkTrimCommandPool, &params);
}

void WINAPI vkTrimCommandPoolKHR(VkDevice device, VkCommandPool commandPool, VkCommandPoolTrimFlags flags)
{
    struct vkTrimCommandPoolKHR_params params;
    params.device = device;
    params.commandPool = commandPool;
    params.flags = flags;
    vk_unix_call(unix_vkTrimCommandPoolKHR, &params);
}

void WINAPI vkUninitializePerformanceApiINTEL(VkDevice device)
{
    struct vkUninitializePerformanceApiINTEL_params params;
    params.device = device;
    vk_unix_call(unix_vkUninitializePerformanceApiINTEL, &params);
}

void WINAPI vkUnmapMemory(VkDevice device, VkDeviceMemory memory)
{
    struct vkUnmapMemory_params params;
    params.device = device;
    params.memory = memory;
    vk_unix_call(unix_vkUnmapMemory, &params);
}

void WINAPI vkUpdateDescriptorSetWithTemplate(VkDevice device, VkDescriptorSet descriptorSet, VkDescriptorUpdateTemplate descriptorUpdateTemplate, const void *pData)
{
    struct vkUpdateDescriptorSetWithTemplate_params params;
    params.device = device;
    params.descriptorSet = descriptorSet;
    params.descriptorUpdateTemplate = descriptorUpdateTemplate;
    params.pData = pData;
    p_vk_direct_unix_call(unix_handle, unix_vkUpdateDescriptorSetWithTemplate, &params);
}

void WINAPI vkUpdateDescriptorSetWithTemplateKHR(VkDevice device, VkDescriptorSet descriptorSet, VkDescriptorUpdateTemplate descriptorUpdateTemplate, const void *pData)
{
    struct vkUpdateDescriptorSetWithTemplateKHR_params params;
    params.device = device;
    params.descriptorSet = descriptorSet;
    params.descriptorUpdateTemplate = descriptorUpdateTemplate;
    params.pData = pData;
    vk_unix_call(unix_vkUpdateDescriptorSetWithTemplateKHR, &params);
}

void WINAPI vkUpdateDescriptorSets(VkDevice device, uint32_t descriptorWriteCount, const VkWriteDescriptorSet *pDescriptorWrites, uint32_t descriptorCopyCount, const VkCopyDescriptorSet *pDescriptorCopies)
{
    struct vkUpdateDescriptorSets_params params;
    params.device = device;
    params.descriptorWriteCount = descriptorWriteCount;
    params.pDescriptorWrites = pDescriptorWrites;
    params.descriptorCopyCount = descriptorCopyCount;
    params.pDescriptorCopies = pDescriptorCopies;
    p_vk_direct_unix_call(unix_handle, unix_vkUpdateDescriptorSets, &params);
}

VkResult WINAPI vkWaitForFences(VkDevice device, uint32_t fenceCount, const VkFence *pFences, VkBool32 waitAll, uint64_t timeout)
{
    struct vkWaitForFences_params params;
    params.device = device;
    params.fenceCount = fenceCount;
    params.pFences = pFences;
    params.waitAll = waitAll;
    params.timeout = timeout;
    vk_unix_call(unix_vkWaitForFences, &params);
    return params.result;
}

VkResult WINAPI vkWaitForPresentKHR(VkDevice device, VkSwapchainKHR swapchain, uint64_t presentId, uint64_t timeout)
{
    struct vkWaitForPresentKHR_params params;
    params.device = device;
    params.swapchain = swapchain;
    params.presentId = presentId;
    params.timeout = timeout;
    vk_unix_call(unix_vkWaitForPresentKHR, &params);
    return params.result;
}

VkResult WINAPI vkWaitSemaphores(VkDevice device, const VkSemaphoreWaitInfo *pWaitInfo, uint64_t timeout)
{
    struct vkWaitSemaphores_params params;
    params.device = device;
    params.pWaitInfo = pWaitInfo;
    params.timeout = timeout;
    vk_unix_call(unix_vkWaitSemaphores, &params);
    return params.result;
}

VkResult WINAPI vkWaitSemaphoresKHR(VkDevice device, const VkSemaphoreWaitInfo *pWaitInfo, uint64_t timeout)
{
    struct vkWaitSemaphoresKHR_params params;
    params.device = device;
    params.pWaitInfo = pWaitInfo;
    params.timeout = timeout;
    vk_unix_call(unix_vkWaitSemaphoresKHR, &params);
    return params.result;
}

VkResult WINAPI vkWriteAccelerationStructuresPropertiesKHR(VkDevice device, uint32_t accelerationStructureCount, const VkAccelerationStructureKHR *pAccelerationStructures, VkQueryType queryType, size_t dataSize, void *pData, size_t stride)
{
    struct vkWriteAccelerationStructuresPropertiesKHR_params params;
    params.device = device;
    params.accelerationStructureCount = accelerationStructureCount;
    params.pAccelerationStructures = pAccelerationStructures;
    params.queryType = queryType;
    params.dataSize = dataSize;
    params.pData = pData;
    params.stride = stride;
    vk_unix_call(unix_vkWriteAccelerationStructuresPropertiesKHR, &params);
    return params.result;
}

VkResult WINAPI vkWriteMicromapsPropertiesEXT(VkDevice device, uint32_t micromapCount, const VkMicromapEXT *pMicromaps, VkQueryType queryType, size_t dataSize, void *pData, size_t stride)
{
    struct vkWriteMicromapsPropertiesEXT_params params;
    params.device = device;
    params.micromapCount = micromapCount;
    params.pMicromaps = pMicromaps;
    params.queryType = queryType;
    params.dataSize = dataSize;
    params.pData = pData;
    params.stride = stride;
    vk_unix_call(unix_vkWriteMicromapsPropertiesEXT, &params);
    return params.result;
}

static const struct vulkan_func vk_device_dispatch_table[] =
{
    {"vkAcquireNextImage2KHR", vkAcquireNextImage2KHR},
    {"vkAcquireNextImageKHR", vkAcquireNextImageKHR},
    {"vkAcquirePerformanceConfigurationINTEL", vkAcquirePerformanceConfigurationINTEL},
    {"vkAcquireProfilingLockKHR", vkAcquireProfilingLockKHR},
    {"vkAllocateCommandBuffers", vkAllocateCommandBuffers},
    {"vkAllocateDescriptorSets", vkAllocateDescriptorSets},
    {"vkAllocateMemory", vkAllocateMemory},
    {"vkBeginCommandBuffer", vkBeginCommandBuffer},
    {"vkBindAccelerationStructureMemoryNV", vkBindAccelerationStructureMemoryNV},
    {"vkBindBufferMemory", vkBindBufferMemory},
    {"vkBindBufferMemory2", vkBindBufferMemory2},
    {"vkBindBufferMemory2KHR", vkBindBufferMemory2KHR},
    {"vkBindImageMemory", vkBindImageMemory},
    {"vkBindImageMemory2", vkBindImageMemory2},
    {"vkBindImageMemory2KHR", vkBindImageMemory2KHR},
    {"vkBindOpticalFlowSessionImageNV", vkBindOpticalFlowSessionImageNV},
    {"vkBuildAccelerationStructuresKHR", vkBuildAccelerationStructuresKHR},
    {"vkBuildMicromapsEXT", vkBuildMicromapsEXT},
    {"vkCmdBeginConditionalRenderingEXT", vkCmdBeginConditionalRenderingEXT},
    {"vkCmdBeginDebugUtilsLabelEXT", vkCmdBeginDebugUtilsLabelEXT},
    {"vkCmdBeginQuery", vkCmdBeginQuery},
    {"vkCmdBeginQueryIndexedEXT", vkCmdBeginQueryIndexedEXT},
    {"vkCmdBeginRenderPass", vkCmdBeginRenderPass},
    {"vkCmdBeginRenderPass2", vkCmdBeginRenderPass2},
    {"vkCmdBeginRenderPass2KHR", vkCmdBeginRenderPass2KHR},
    {"vkCmdBeginRendering", vkCmdBeginRendering},
    {"vkCmdBeginRenderingKHR", vkCmdBeginRenderingKHR},
    {"vkCmdBeginTransformFeedbackEXT", vkCmdBeginTransformFeedbackEXT},
    {"vkCmdBindDescriptorSets", vkCmdBindDescriptorSets},
    {"vkCmdBindIndexBuffer", vkCmdBindIndexBuffer},
    {"vkCmdBindInvocationMaskHUAWEI", vkCmdBindInvocationMaskHUAWEI},
    {"vkCmdBindPipeline", vkCmdBindPipeline},
    {"vkCmdBindPipelineShaderGroupNV", vkCmdBindPipelineShaderGroupNV},
    {"vkCmdBindShadingRateImageNV", vkCmdBindShadingRateImageNV},
    {"vkCmdBindTransformFeedbackBuffersEXT", vkCmdBindTransformFeedbackBuffersEXT},
    {"vkCmdBindVertexBuffers", vkCmdBindVertexBuffers},
    {"vkCmdBindVertexBuffers2", vkCmdBindVertexBuffers2},
    {"vkCmdBindVertexBuffers2EXT", vkCmdBindVertexBuffers2EXT},
    {"vkCmdBlitImage", vkCmdBlitImage},
    {"vkCmdBlitImage2", vkCmdBlitImage2},
    {"vkCmdBlitImage2KHR", vkCmdBlitImage2KHR},
    {"vkCmdBuildAccelerationStructureNV", vkCmdBuildAccelerationStructureNV},
    {"vkCmdBuildAccelerationStructuresIndirectKHR", vkCmdBuildAccelerationStructuresIndirectKHR},
    {"vkCmdBuildAccelerationStructuresKHR", vkCmdBuildAccelerationStructuresKHR},
    {"vkCmdBuildMicromapsEXT", vkCmdBuildMicromapsEXT},
    {"vkCmdClearAttachments", vkCmdClearAttachments},
    {"vkCmdClearColorImage", vkCmdClearColorImage},
    {"vkCmdClearDepthStencilImage", vkCmdClearDepthStencilImage},
    {"vkCmdCopyAccelerationStructureKHR", vkCmdCopyAccelerationStructureKHR},
    {"vkCmdCopyAccelerationStructureNV", vkCmdCopyAccelerationStructureNV},
    {"vkCmdCopyAccelerationStructureToMemoryKHR", vkCmdCopyAccelerationStructureToMemoryKHR},
    {"vkCmdCopyBuffer", vkCmdCopyBuffer},
    {"vkCmdCopyBuffer2", vkCmdCopyBuffer2},
    {"vkCmdCopyBuffer2KHR", vkCmdCopyBuffer2KHR},
    {"vkCmdCopyBufferToImage", vkCmdCopyBufferToImage},
    {"vkCmdCopyBufferToImage2", vkCmdCopyBufferToImage2},
    {"vkCmdCopyBufferToImage2KHR", vkCmdCopyBufferToImage2KHR},
    {"vkCmdCopyImage", vkCmdCopyImage},
    {"vkCmdCopyImage2", vkCmdCopyImage2},
    {"vkCmdCopyImage2KHR", vkCmdCopyImage2KHR},
    {"vkCmdCopyImageToBuffer", vkCmdCopyImageToBuffer},
    {"vkCmdCopyImageToBuffer2", vkCmdCopyImageToBuffer2},
    {"vkCmdCopyImageToBuffer2KHR", vkCmdCopyImageToBuffer2KHR},
    {"vkCmdCopyMemoryToAccelerationStructureKHR", vkCmdCopyMemoryToAccelerationStructureKHR},
    {"vkCmdCopyMemoryToMicromapEXT", vkCmdCopyMemoryToMicromapEXT},
    {"vkCmdCopyMicromapEXT", vkCmdCopyMicromapEXT},
    {"vkCmdCopyMicromapToMemoryEXT", vkCmdCopyMicromapToMemoryEXT},
    {"vkCmdCopyQueryPoolResults", vkCmdCopyQueryPoolResults},
    {"vkCmdCuLaunchKernelNVX", vkCmdCuLaunchKernelNVX},
    {"vkCmdDebugMarkerBeginEXT", vkCmdDebugMarkerBeginEXT},
    {"vkCmdDebugMarkerEndEXT", vkCmdDebugMarkerEndEXT},
    {"vkCmdDebugMarkerInsertEXT", vkCmdDebugMarkerInsertEXT},
    {"vkCmdDispatch", vkCmdDispatch},
    {"vkCmdDispatchBase", vkCmdDispatchBase},
    {"vkCmdDispatchBaseKHR", vkCmdDispatchBaseKHR},
    {"vkCmdDispatchIndirect", vkCmdDispatchIndirect},
    {"vkCmdDraw", vkCmdDraw},
    {"vkCmdDrawIndexed", vkCmdDrawIndexed},
    {"vkCmdDrawIndexedIndirect", vkCmdDrawIndexedIndirect},
    {"vkCmdDrawIndexedIndirectCount", vkCmdDrawIndexedIndirectCount},
    {"vkCmdDrawIndexedIndirectCountAMD", vkCmdDrawIndexedIndirectCountAMD},
    {"vkCmdDrawIndexedIndirectCountKHR", vkCmdDrawIndexedIndirectCountKHR},
    {"vkCmdDrawIndirect", vkCmdDrawIndirect},
    {"vkCmdDrawIndirectByteCountEXT", vkCmdDrawIndirectByteCountEXT},
    {"vkCmdDrawIndirectCount", vkCmdDrawIndirectCount},
    {"vkCmdDrawIndirectCountAMD", vkCmdDrawIndirectCountAMD},
    {"vkCmdDrawIndirectCountKHR", vkCmdDrawIndirectCountKHR},
    {"vkCmdDrawMeshTasksEXT", vkCmdDrawMeshTasksEXT},
    {"vkCmdDrawMeshTasksIndirectCountEXT", vkCmdDrawMeshTasksIndirectCountEXT},
    {"vkCmdDrawMeshTasksIndirectCountNV", vkCmdDrawMeshTasksIndirectCountNV},
    {"vkCmdDrawMeshTasksIndirectEXT", vkCmdDrawMeshTasksIndirectEXT},
    {"vkCmdDrawMeshTasksIndirectNV", vkCmdDrawMeshTasksIndirectNV},
    {"vkCmdDrawMeshTasksNV", vkCmdDrawMeshTasksNV},
    {"vkCmdDrawMultiEXT", vkCmdDrawMultiEXT},
    {"vkCmdDrawMultiIndexedEXT", vkCmdDrawMultiIndexedEXT},
    {"vkCmdEndConditionalRenderingEXT", vkCmdEndConditionalRenderingEXT},
    {"vkCmdEndDebugUtilsLabelEXT", vkCmdEndDebugUtilsLabelEXT},
    {"vkCmdEndQuery", vkCmdEndQuery},
    {"vkCmdEndQueryIndexedEXT", vkCmdEndQueryIndexedEXT},
    {"vkCmdEndRenderPass", vkCmdEndRenderPass},
    {"vkCmdEndRenderPass2", vkCmdEndRenderPass2},
    {"vkCmdEndRenderPass2KHR", vkCmdEndRenderPass2KHR},
    {"vkCmdEndRendering", vkCmdEndRendering},
    {"vkCmdEndRenderingKHR", vkCmdEndRenderingKHR},
    {"vkCmdEndTransformFeedbackEXT", vkCmdEndTransformFeedbackEXT},
    {"vkCmdExecuteCommands", vkCmdExecuteCommands},
    {"vkCmdExecuteGeneratedCommandsNV", vkCmdExecuteGeneratedCommandsNV},
    {"vkCmdFillBuffer", vkCmdFillBuffer},
    {"vkCmdInsertDebugUtilsLabelEXT", vkCmdInsertDebugUtilsLabelEXT},
    {"vkCmdNextSubpass", vkCmdNextSubpass},
    {"vkCmdNextSubpass2", vkCmdNextSubpass2},
    {"vkCmdNextSubpass2KHR", vkCmdNextSubpass2KHR},
    {"vkCmdOpticalFlowExecuteNV", vkCmdOpticalFlowExecuteNV},
    {"vkCmdPipelineBarrier", vkCmdPipelineBarrier},
    {"vkCmdPipelineBarrier2", vkCmdPipelineBarrier2},
    {"vkCmdPipelineBarrier2KHR", vkCmdPipelineBarrier2KHR},
    {"vkCmdPreprocessGeneratedCommandsNV", vkCmdPreprocessGeneratedCommandsNV},
    {"vkCmdPushConstants", vkCmdPushConstants},
    {"vkCmdPushDescriptorSetKHR", vkCmdPushDescriptorSetKHR},
    {"vkCmdPushDescriptorSetWithTemplateKHR", vkCmdPushDescriptorSetWithTemplateKHR},
    {"vkCmdResetEvent", vkCmdResetEvent},
    {"vkCmdResetEvent2", vkCmdResetEvent2},
    {"vkCmdResetEvent2KHR", vkCmdResetEvent2KHR},
    {"vkCmdResetQueryPool", vkCmdResetQueryPool},
    {"vkCmdResolveImage", vkCmdResolveImage},
    {"vkCmdResolveImage2", vkCmdResolveImage2},
    {"vkCmdResolveImage2KHR", vkCmdResolveImage2KHR},
    {"vkCmdSetAlphaToCoverageEnableEXT", vkCmdSetAlphaToCoverageEnableEXT},
    {"vkCmdSetAlphaToOneEnableEXT", vkCmdSetAlphaToOneEnableEXT},
    {"vkCmdSetBlendConstants", vkCmdSetBlendConstants},
    {"vkCmdSetCheckpointNV", vkCmdSetCheckpointNV},
    {"vkCmdSetCoarseSampleOrderNV", vkCmdSetCoarseSampleOrderNV},
    {"vkCmdSetColorBlendAdvancedEXT", vkCmdSetColorBlendAdvancedEXT},
    {"vkCmdSetColorBlendEnableEXT", vkCmdSetColorBlendEnableEXT},
    {"vkCmdSetColorBlendEquationEXT", vkCmdSetColorBlendEquationEXT},
    {"vkCmdSetColorWriteEnableEXT", vkCmdSetColorWriteEnableEXT},
    {"vkCmdSetColorWriteMaskEXT", vkCmdSetColorWriteMaskEXT},
    {"vkCmdSetConservativeRasterizationModeEXT", vkCmdSetConservativeRasterizationModeEXT},
    {"vkCmdSetCoverageModulationModeNV", vkCmdSetCoverageModulationModeNV},
    {"vkCmdSetCoverageModulationTableEnableNV", vkCmdSetCoverageModulationTableEnableNV},
    {"vkCmdSetCoverageModulationTableNV", vkCmdSetCoverageModulationTableNV},
    {"vkCmdSetCoverageReductionModeNV", vkCmdSetCoverageReductionModeNV},
    {"vkCmdSetCoverageToColorEnableNV", vkCmdSetCoverageToColorEnableNV},
    {"vkCmdSetCoverageToColorLocationNV", vkCmdSetCoverageToColorLocationNV},
    {"vkCmdSetCullMode", vkCmdSetCullMode},
    {"vkCmdSetCullModeEXT", vkCmdSetCullModeEXT},
    {"vkCmdSetDepthBias", vkCmdSetDepthBias},
    {"vkCmdSetDepthBiasEnable", vkCmdSetDepthBiasEnable},
    {"vkCmdSetDepthBiasEnableEXT", vkCmdSetDepthBiasEnableEXT},
    {"vkCmdSetDepthBounds", vkCmdSetDepthBounds},
    {"vkCmdSetDepthBoundsTestEnable", vkCmdSetDepthBoundsTestEnable},
    {"vkCmdSetDepthBoundsTestEnableEXT", vkCmdSetDepthBoundsTestEnableEXT},
    {"vkCmdSetDepthClampEnableEXT", vkCmdSetDepthClampEnableEXT},
    {"vkCmdSetDepthClipEnableEXT", vkCmdSetDepthClipEnableEXT},
    {"vkCmdSetDepthClipNegativeOneToOneEXT", vkCmdSetDepthClipNegativeOneToOneEXT},
    {"vkCmdSetDepthCompareOp", vkCmdSetDepthCompareOp},
    {"vkCmdSetDepthCompareOpEXT", vkCmdSetDepthCompareOpEXT},
    {"vkCmdSetDepthTestEnable", vkCmdSetDepthTestEnable},
    {"vkCmdSetDepthTestEnableEXT", vkCmdSetDepthTestEnableEXT},
    {"vkCmdSetDepthWriteEnable", vkCmdSetDepthWriteEnable},
    {"vkCmdSetDepthWriteEnableEXT", vkCmdSetDepthWriteEnableEXT},
    {"vkCmdSetDeviceMask", vkCmdSetDeviceMask},
    {"vkCmdSetDeviceMaskKHR", vkCmdSetDeviceMaskKHR},
    {"vkCmdSetDiscardRectangleEXT", vkCmdSetDiscardRectangleEXT},
    {"vkCmdSetEvent", vkCmdSetEvent},
    {"vkCmdSetEvent2", vkCmdSetEvent2},
    {"vkCmdSetEvent2KHR", vkCmdSetEvent2KHR},
    {"vkCmdSetExclusiveScissorNV", vkCmdSetExclusiveScissorNV},
    {"vkCmdSetExtraPrimitiveOverestimationSizeEXT", vkCmdSetExtraPrimitiveOverestimationSizeEXT},
    {"vkCmdSetFragmentShadingRateEnumNV", vkCmdSetFragmentShadingRateEnumNV},
    {"vkCmdSetFragmentShadingRateKHR", vkCmdSetFragmentShadingRateKHR},
    {"vkCmdSetFrontFace", vkCmdSetFrontFace},
    {"vkCmdSetFrontFaceEXT", vkCmdSetFrontFaceEXT},
    {"vkCmdSetLineRasterizationModeEXT", vkCmdSetLineRasterizationModeEXT},
    {"vkCmdSetLineStippleEXT", vkCmdSetLineStippleEXT},
    {"vkCmdSetLineStippleEnableEXT", vkCmdSetLineStippleEnableEXT},
    {"vkCmdSetLineWidth", vkCmdSetLineWidth},
    {"vkCmdSetLogicOpEXT", vkCmdSetLogicOpEXT},
    {"vkCmdSetLogicOpEnableEXT", vkCmdSetLogicOpEnableEXT},
    {"vkCmdSetPatchControlPointsEXT", vkCmdSetPatchControlPointsEXT},
    {"vkCmdSetPerformanceMarkerINTEL", vkCmdSetPerformanceMarkerINTEL},
    {"vkCmdSetPerformanceOverrideINTEL", vkCmdSetPerformanceOverrideINTEL},
    {"vkCmdSetPerformanceStreamMarkerINTEL", vkCmdSetPerformanceStreamMarkerINTEL},
    {"vkCmdSetPolygonModeEXT", vkCmdSetPolygonModeEXT},
    {"vkCmdSetPrimitiveRestartEnable", vkCmdSetPrimitiveRestartEnable},
    {"vkCmdSetPrimitiveRestartEnableEXT", vkCmdSetPrimitiveRestartEnableEXT},
    {"vkCmdSetPrimitiveTopology", vkCmdSetPrimitiveTopology},
    {"vkCmdSetPrimitiveTopologyEXT", vkCmdSetPrimitiveTopologyEXT},
    {"vkCmdSetProvokingVertexModeEXT", vkCmdSetProvokingVertexModeEXT},
    {"vkCmdSetRasterizationSamplesEXT", vkCmdSetRasterizationSamplesEXT},
    {"vkCmdSetRasterizationStreamEXT", vkCmdSetRasterizationStreamEXT},
    {"vkCmdSetRasterizerDiscardEnable", vkCmdSetRasterizerDiscardEnable},
    {"vkCmdSetRasterizerDiscardEnableEXT", vkCmdSetRasterizerDiscardEnableEXT},
    {"vkCmdSetRayTracingPipelineStackSizeKHR", vkCmdSetRayTracingPipelineStackSizeKHR},
    {"vkCmdSetRepresentativeFragmentTestEnableNV", vkCmdSetRepresentativeFragmentTestEnableNV},
    {"vkCmdSetSampleLocationsEXT", vkCmdSetSampleLocationsEXT},
    {"vkCmdSetSampleLocationsEnableEXT", vkCmdSetSampleLocationsEnableEXT},
    {"vkCmdSetSampleMaskEXT", vkCmdSetSampleMaskEXT},
    {"vkCmdSetScissor", vkCmdSetScissor},
    {"vkCmdSetScissorWithCount", vkCmdSetScissorWithCount},
    {"vkCmdSetScissorWithCountEXT", vkCmdSetScissorWithCountEXT},
    {"vkCmdSetShadingRateImageEnableNV", vkCmdSetShadingRateImageEnableNV},
    {"vkCmdSetStencilCompareMask", vkCmdSetStencilCompareMask},
    {"vkCmdSetStencilOp", vkCmdSetStencilOp},
    {"vkCmdSetStencilOpEXT", vkCmdSetStencilOpEXT},
    {"vkCmdSetStencilReference", vkCmdSetStencilReference},
    {"vkCmdSetStencilTestEnable", vkCmdSetStencilTestEnable},
    {"vkCmdSetStencilTestEnableEXT", vkCmdSetStencilTestEnableEXT},
    {"vkCmdSetStencilWriteMask", vkCmdSetStencilWriteMask},
    {"vkCmdSetTessellationDomainOriginEXT", vkCmdSetTessellationDomainOriginEXT},
    {"vkCmdSetVertexInputEXT", vkCmdSetVertexInputEXT},
    {"vkCmdSetViewport", vkCmdSetViewport},
    {"vkCmdSetViewportShadingRatePaletteNV", vkCmdSetViewportShadingRatePaletteNV},
    {"vkCmdSetViewportSwizzleNV", vkCmdSetViewportSwizzleNV},
    {"vkCmdSetViewportWScalingEnableNV", vkCmdSetViewportWScalingEnableNV},
    {"vkCmdSetViewportWScalingNV", vkCmdSetViewportWScalingNV},
    {"vkCmdSetViewportWithCount", vkCmdSetViewportWithCount},
    {"vkCmdSetViewportWithCountEXT", vkCmdSetViewportWithCountEXT},
    {"vkCmdSubpassShadingHUAWEI", vkCmdSubpassShadingHUAWEI},
    {"vkCmdTraceRaysIndirect2KHR", vkCmdTraceRaysIndirect2KHR},
    {"vkCmdTraceRaysIndirectKHR", vkCmdTraceRaysIndirectKHR},
    {"vkCmdTraceRaysKHR", vkCmdTraceRaysKHR},
    {"vkCmdTraceRaysNV", vkCmdTraceRaysNV},
    {"vkCmdUpdateBuffer", vkCmdUpdateBuffer},
    {"vkCmdWaitEvents", vkCmdWaitEvents},
    {"vkCmdWaitEvents2", vkCmdWaitEvents2},
    {"vkCmdWaitEvents2KHR", vkCmdWaitEvents2KHR},
    {"vkCmdWriteAccelerationStructuresPropertiesKHR", vkCmdWriteAccelerationStructuresPropertiesKHR},
    {"vkCmdWriteAccelerationStructuresPropertiesNV", vkCmdWriteAccelerationStructuresPropertiesNV},
    {"vkCmdWriteBufferMarker2AMD", vkCmdWriteBufferMarker2AMD},
    {"vkCmdWriteBufferMarkerAMD", vkCmdWriteBufferMarkerAMD},
    {"vkCmdWriteMicromapsPropertiesEXT", vkCmdWriteMicromapsPropertiesEXT},
    {"vkCmdWriteTimestamp", vkCmdWriteTimestamp},
    {"vkCmdWriteTimestamp2", vkCmdWriteTimestamp2},
    {"vkCmdWriteTimestamp2KHR", vkCmdWriteTimestamp2KHR},
    {"vkCompileDeferredNV", vkCompileDeferredNV},
    {"vkCopyAccelerationStructureKHR", vkCopyAccelerationStructureKHR},
    {"vkCopyAccelerationStructureToMemoryKHR", vkCopyAccelerationStructureToMemoryKHR},
    {"vkCopyMemoryToAccelerationStructureKHR", vkCopyMemoryToAccelerationStructureKHR},
    {"vkCopyMemoryToMicromapEXT", vkCopyMemoryToMicromapEXT},
    {"vkCopyMicromapEXT", vkCopyMicromapEXT},
    {"vkCopyMicromapToMemoryEXT", vkCopyMicromapToMemoryEXT},
    {"vkCreateAccelerationStructureKHR", vkCreateAccelerationStructureKHR},
    {"vkCreateAccelerationStructureNV", vkCreateAccelerationStructureNV},
    {"vkCreateBuffer", vkCreateBuffer},
    {"vkCreateBufferView", vkCreateBufferView},
    {"vkCreateCommandPool", vkCreateCommandPool},
    {"vkCreateComputePipelines", vkCreateComputePipelines},
    {"vkCreateCuFunctionNVX", vkCreateCuFunctionNVX},
    {"vkCreateCuModuleNVX", vkCreateCuModuleNVX},
    {"vkCreateDeferredOperationKHR", vkCreateDeferredOperationKHR},
    {"vkCreateDescriptorPool", vkCreateDescriptorPool},
    {"vkCreateDescriptorSetLayout", vkCreateDescriptorSetLayout},
    {"vkCreateDescriptorUpdateTemplate", vkCreateDescriptorUpdateTemplate},
    {"vkCreateDescriptorUpdateTemplateKHR", vkCreateDescriptorUpdateTemplateKHR},
    {"vkCreateEvent", vkCreateEvent},
    {"vkCreateFence", vkCreateFence},
    {"vkCreateFramebuffer", vkCreateFramebuffer},
    {"vkCreateGraphicsPipelines", vkCreateGraphicsPipelines},
    {"vkCreateImage", vkCreateImage},
    {"vkCreateImageView", vkCreateImageView},
    {"vkCreateIndirectCommandsLayoutNV", vkCreateIndirectCommandsLayoutNV},
    {"vkCreateMicromapEXT", vkCreateMicromapEXT},
    {"vkCreateOpticalFlowSessionNV", vkCreateOpticalFlowSessionNV},
    {"vkCreatePipelineCache", vkCreatePipelineCache},
    {"vkCreatePipelineLayout", vkCreatePipelineLayout},
    {"vkCreatePrivateDataSlot", vkCreatePrivateDataSlot},
    {"vkCreatePrivateDataSlotEXT", vkCreatePrivateDataSlotEXT},
    {"vkCreateQueryPool", vkCreateQueryPool},
    {"vkCreateRayTracingPipelinesKHR", vkCreateRayTracingPipelinesKHR},
    {"vkCreateRayTracingPipelinesNV", vkCreateRayTracingPipelinesNV},
    {"vkCreateRenderPass", vkCreateRenderPass},
    {"vkCreateRenderPass2", vkCreateRenderPass2},
    {"vkCreateRenderPass2KHR", vkCreateRenderPass2KHR},
    {"vkCreateSampler", vkCreateSampler},
    {"vkCreateSamplerYcbcrConversion", vkCreateSamplerYcbcrConversion},
    {"vkCreateSamplerYcbcrConversionKHR", vkCreateSamplerYcbcrConversionKHR},
    {"vkCreateSemaphore", vkCreateSemaphore},
    {"vkCreateShaderModule", vkCreateShaderModule},
    {"vkCreateSwapchainKHR", vkCreateSwapchainKHR},
    {"vkCreateValidationCacheEXT", vkCreateValidationCacheEXT},
    {"vkDebugMarkerSetObjectNameEXT", vkDebugMarkerSetObjectNameEXT},
    {"vkDebugMarkerSetObjectTagEXT", vkDebugMarkerSetObjectTagEXT},
    {"vkDeferredOperationJoinKHR", vkDeferredOperationJoinKHR},
    {"vkDestroyAccelerationStructureKHR", vkDestroyAccelerationStructureKHR},
    {"vkDestroyAccelerationStructureNV", vkDestroyAccelerationStructureNV},
    {"vkDestroyBuffer", vkDestroyBuffer},
    {"vkDestroyBufferView", vkDestroyBufferView},
    {"vkDestroyCommandPool", vkDestroyCommandPool},
    {"vkDestroyCuFunctionNVX", vkDestroyCuFunctionNVX},
    {"vkDestroyCuModuleNVX", vkDestroyCuModuleNVX},
    {"vkDestroyDeferredOperationKHR", vkDestroyDeferredOperationKHR},
    {"vkDestroyDescriptorPool", vkDestroyDescriptorPool},
    {"vkDestroyDescriptorSetLayout", vkDestroyDescriptorSetLayout},
    {"vkDestroyDescriptorUpdateTemplate", vkDestroyDescriptorUpdateTemplate},
    {"vkDestroyDescriptorUpdateTemplateKHR", vkDestroyDescriptorUpdateTemplateKHR},
    {"vkDestroyDevice", vkDestroyDevice},
    {"vkDestroyEvent", vkDestroyEvent},
    {"vkDestroyFence", vkDestroyFence},
    {"vkDestroyFramebuffer", vkDestroyFramebuffer},
    {"vkDestroyImage", vkDestroyImage},
    {"vkDestroyImageView", vkDestroyImageView},
    {"vkDestroyIndirectCommandsLayoutNV", vkDestroyIndirectCommandsLayoutNV},
    {"vkDestroyMicromapEXT", vkDestroyMicromapEXT},
    {"vkDestroyOpticalFlowSessionNV", vkDestroyOpticalFlowSessionNV},
    {"vkDestroyPipeline", vkDestroyPipeline},
    {"vkDestroyPipelineCache", vkDestroyPipelineCache},
    {"vkDestroyPipelineLayout", vkDestroyPipelineLayout},
    {"vkDestroyPrivateDataSlot", vkDestroyPrivateDataSlot},
    {"vkDestroyPrivateDataSlotEXT", vkDestroyPrivateDataSlotEXT},
    {"vkDestroyQueryPool", vkDestroyQueryPool},
    {"vkDestroyRenderPass", vkDestroyRenderPass},
    {"vkDestroySampler", vkDestroySampler},
    {"vkDestroySamplerYcbcrConversion", vkDestroySamplerYcbcrConversion},
    {"vkDestroySamplerYcbcrConversionKHR", vkDestroySamplerYcbcrConversionKHR},
    {"vkDestroySemaphore", vkDestroySemaphore},
    {"vkDestroyShaderModule", vkDestroyShaderModule},
    {"vkDestroySwapchainKHR", vkDestroySwapchainKHR},
    {"vkDestroyValidationCacheEXT", vkDestroyValidationCacheEXT},
    {"vkDeviceWaitIdle", vkDeviceWaitIdle},
    {"vkEndCommandBuffer", vkEndCommandBuffer},
    {"vkFlushMappedMemoryRanges", vkFlushMappedMemoryRanges},
    {"vkFreeCommandBuffers", vkFreeCommandBuffers},
    {"vkFreeDescriptorSets", vkFreeDescriptorSets},
    {"vkFreeMemory", vkFreeMemory},
    {"vkGetAccelerationStructureBuildSizesKHR", vkGetAccelerationStructureBuildSizesKHR},
    {"vkGetAccelerationStructureDeviceAddressKHR", vkGetAccelerationStructureDeviceAddressKHR},
    {"vkGetAccelerationStructureHandleNV", vkGetAccelerationStructureHandleNV},
    {"vkGetAccelerationStructureMemoryRequirementsNV", vkGetAccelerationStructureMemoryRequirementsNV},
    {"vkGetBufferDeviceAddress", vkGetBufferDeviceAddress},
    {"vkGetBufferDeviceAddressEXT", vkGetBufferDeviceAddressEXT},
    {"vkGetBufferDeviceAddressKHR", vkGetBufferDeviceAddressKHR},
    {"vkGetBufferMemoryRequirements", vkGetBufferMemoryRequirements},
    {"vkGetBufferMemoryRequirements2", vkGetBufferMemoryRequirements2},
    {"vkGetBufferMemoryRequirements2KHR", vkGetBufferMemoryRequirements2KHR},
    {"vkGetBufferOpaqueCaptureAddress", vkGetBufferOpaqueCaptureAddress},
    {"vkGetBufferOpaqueCaptureAddressKHR", vkGetBufferOpaqueCaptureAddressKHR},
    {"vkGetCalibratedTimestampsEXT", vkGetCalibratedTimestampsEXT},
    {"vkGetDeferredOperationMaxConcurrencyKHR", vkGetDeferredOperationMaxConcurrencyKHR},
    {"vkGetDeferredOperationResultKHR", vkGetDeferredOperationResultKHR},
    {"vkGetDescriptorSetHostMappingVALVE", vkGetDescriptorSetHostMappingVALVE},
    {"vkGetDescriptorSetLayoutHostMappingInfoVALVE", vkGetDescriptorSetLayoutHostMappingInfoVALVE},
    {"vkGetDescriptorSetLayoutSupport", vkGetDescriptorSetLayoutSupport},
    {"vkGetDescriptorSetLayoutSupportKHR", vkGetDescriptorSetLayoutSupportKHR},
    {"vkGetDeviceAccelerationStructureCompatibilityKHR", vkGetDeviceAccelerationStructureCompatibilityKHR},
    {"vkGetDeviceBufferMemoryRequirements", vkGetDeviceBufferMemoryRequirements},
    {"vkGetDeviceBufferMemoryRequirementsKHR", vkGetDeviceBufferMemoryRequirementsKHR},
    {"vkGetDeviceFaultInfoEXT", vkGetDeviceFaultInfoEXT},
    {"vkGetDeviceGroupPeerMemoryFeatures", vkGetDeviceGroupPeerMemoryFeatures},
    {"vkGetDeviceGroupPeerMemoryFeaturesKHR", vkGetDeviceGroupPeerMemoryFeaturesKHR},
    {"vkGetDeviceGroupPresentCapabilitiesKHR", vkGetDeviceGroupPresentCapabilitiesKHR},
    {"vkGetDeviceGroupSurfacePresentModesKHR", vkGetDeviceGroupSurfacePresentModesKHR},
    {"vkGetDeviceImageMemoryRequirements", vkGetDeviceImageMemoryRequirements},
    {"vkGetDeviceImageMemoryRequirementsKHR", vkGetDeviceImageMemoryRequirementsKHR},
    {"vkGetDeviceImageSparseMemoryRequirements", vkGetDeviceImageSparseMemoryRequirements},
    {"vkGetDeviceImageSparseMemoryRequirementsKHR", vkGetDeviceImageSparseMemoryRequirementsKHR},
    {"vkGetDeviceMemoryCommitment", vkGetDeviceMemoryCommitment},
    {"vkGetDeviceMemoryOpaqueCaptureAddress", vkGetDeviceMemoryOpaqueCaptureAddress},
    {"vkGetDeviceMemoryOpaqueCaptureAddressKHR", vkGetDeviceMemoryOpaqueCaptureAddressKHR},
    {"vkGetDeviceMicromapCompatibilityEXT", vkGetDeviceMicromapCompatibilityEXT},
    {"vkGetDeviceProcAddr", vkGetDeviceProcAddr},
    {"vkGetDeviceQueue", vkGetDeviceQueue},
    {"vkGetDeviceQueue2", vkGetDeviceQueue2},
    {"vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI", vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI},
    {"vkGetDynamicRenderingTilePropertiesQCOM", vkGetDynamicRenderingTilePropertiesQCOM},
    {"vkGetEventStatus", vkGetEventStatus},
    {"vkGetFenceStatus", vkGetFenceStatus},
    {"vkGetFramebufferTilePropertiesQCOM", vkGetFramebufferTilePropertiesQCOM},
    {"vkGetGeneratedCommandsMemoryRequirementsNV", vkGetGeneratedCommandsMemoryRequirementsNV},
    {"vkGetImageMemoryRequirements", vkGetImageMemoryRequirements},
    {"vkGetImageMemoryRequirements2", vkGetImageMemoryRequirements2},
    {"vkGetImageMemoryRequirements2KHR", vkGetImageMemoryRequirements2KHR},
    {"vkGetImageSparseMemoryRequirements", vkGetImageSparseMemoryRequirements},
    {"vkGetImageSparseMemoryRequirements2", vkGetImageSparseMemoryRequirements2},
    {"vkGetImageSparseMemoryRequirements2KHR", vkGetImageSparseMemoryRequirements2KHR},
    {"vkGetImageSubresourceLayout", vkGetImageSubresourceLayout},
    {"vkGetImageSubresourceLayout2EXT", vkGetImageSubresourceLayout2EXT},
    {"vkGetImageViewAddressNVX", vkGetImageViewAddressNVX},
    {"vkGetImageViewHandleNVX", vkGetImageViewHandleNVX},
    {"vkGetMemoryHostPointerPropertiesEXT", vkGetMemoryHostPointerPropertiesEXT},
    {"vkGetMicromapBuildSizesEXT", vkGetMicromapBuildSizesEXT},
    {"vkGetPerformanceParameterINTEL", vkGetPerformanceParameterINTEL},
    {"vkGetPipelineCacheData", vkGetPipelineCacheData},
    {"vkGetPipelineExecutableInternalRepresentationsKHR", vkGetPipelineExecutableInternalRepresentationsKHR},
    {"vkGetPipelineExecutablePropertiesKHR", vkGetPipelineExecutablePropertiesKHR},
    {"vkGetPipelineExecutableStatisticsKHR", vkGetPipelineExecutableStatisticsKHR},
    {"vkGetPipelinePropertiesEXT", vkGetPipelinePropertiesEXT},
    {"vkGetPrivateData", vkGetPrivateData},
    {"vkGetPrivateDataEXT", vkGetPrivateDataEXT},
    {"vkGetQueryPoolResults", vkGetQueryPoolResults},
    {"vkGetQueueCheckpointData2NV", vkGetQueueCheckpointData2NV},
    {"vkGetQueueCheckpointDataNV", vkGetQueueCheckpointDataNV},
    {"vkGetRayTracingCaptureReplayShaderGroupHandlesKHR", vkGetRayTracingCaptureReplayShaderGroupHandlesKHR},
    {"vkGetRayTracingShaderGroupHandlesKHR", vkGetRayTracingShaderGroupHandlesKHR},
    {"vkGetRayTracingShaderGroupHandlesNV", vkGetRayTracingShaderGroupHandlesNV},
    {"vkGetRayTracingShaderGroupStackSizeKHR", vkGetRayTracingShaderGroupStackSizeKHR},
    {"vkGetRenderAreaGranularity", vkGetRenderAreaGranularity},
    {"vkGetSemaphoreCounterValue", vkGetSemaphoreCounterValue},
    {"vkGetSemaphoreCounterValueKHR", vkGetSemaphoreCounterValueKHR},
    {"vkGetShaderInfoAMD", vkGetShaderInfoAMD},
    {"vkGetShaderModuleCreateInfoIdentifierEXT", vkGetShaderModuleCreateInfoIdentifierEXT},
    {"vkGetShaderModuleIdentifierEXT", vkGetShaderModuleIdentifierEXT},
    {"vkGetSwapchainImagesKHR", vkGetSwapchainImagesKHR},
    {"vkGetValidationCacheDataEXT", vkGetValidationCacheDataEXT},
    {"vkInitializePerformanceApiINTEL", vkInitializePerformanceApiINTEL},
    {"vkInvalidateMappedMemoryRanges", vkInvalidateMappedMemoryRanges},
    {"vkMapMemory", vkMapMemory},
    {"vkMergePipelineCaches", vkMergePipelineCaches},
    {"vkMergeValidationCachesEXT", vkMergeValidationCachesEXT},
    {"vkQueueBeginDebugUtilsLabelEXT", vkQueueBeginDebugUtilsLabelEXT},
    {"vkQueueBindSparse", vkQueueBindSparse},
    {"vkQueueEndDebugUtilsLabelEXT", vkQueueEndDebugUtilsLabelEXT},
    {"vkQueueInsertDebugUtilsLabelEXT", vkQueueInsertDebugUtilsLabelEXT},
    {"vkQueuePresentKHR", vkQueuePresentKHR},
    {"vkQueueSetPerformanceConfigurationINTEL", vkQueueSetPerformanceConfigurationINTEL},
    {"vkQueueSubmit", vkQueueSubmit},
    {"vkQueueSubmit2", vkQueueSubmit2},
    {"vkQueueSubmit2KHR", vkQueueSubmit2KHR},
    {"vkQueueWaitIdle", vkQueueWaitIdle},
    {"vkReleasePerformanceConfigurationINTEL", vkReleasePerformanceConfigurationINTEL},
    {"vkReleaseProfilingLockKHR", vkReleaseProfilingLockKHR},
    {"vkResetCommandBuffer", vkResetCommandBuffer},
    {"vkResetCommandPool", vkResetCommandPool},
    {"vkResetDescriptorPool", vkResetDescriptorPool},
    {"vkResetEvent", vkResetEvent},
    {"vkResetFences", vkResetFences},
    {"vkResetQueryPool", vkResetQueryPool},
    {"vkResetQueryPoolEXT", vkResetQueryPoolEXT},
    {"vkSetDebugUtilsObjectNameEXT", vkSetDebugUtilsObjectNameEXT},
    {"vkSetDebugUtilsObjectTagEXT", vkSetDebugUtilsObjectTagEXT},
    {"vkSetDeviceMemoryPriorityEXT", vkSetDeviceMemoryPriorityEXT},
    {"vkSetEvent", vkSetEvent},
    {"vkSetPrivateData", vkSetPrivateData},
    {"vkSetPrivateDataEXT", vkSetPrivateDataEXT},
    {"vkSignalSemaphore", vkSignalSemaphore},
    {"vkSignalSemaphoreKHR", vkSignalSemaphoreKHR},
    {"vkTrimCommandPool", vkTrimCommandPool},
    {"vkTrimCommandPoolKHR", vkTrimCommandPoolKHR},
    {"vkUninitializePerformanceApiINTEL", vkUninitializePerformanceApiINTEL},
    {"vkUnmapMemory", vkUnmapMemory},
    {"vkUpdateDescriptorSetWithTemplate", vkUpdateDescriptorSetWithTemplate},
    {"vkUpdateDescriptorSetWithTemplateKHR", vkUpdateDescriptorSetWithTemplateKHR},
    {"vkUpdateDescriptorSets", vkUpdateDescriptorSets},
    {"vkWaitForFences", vkWaitForFences},
    {"vkWaitForPresentKHR", vkWaitForPresentKHR},
    {"vkWaitSemaphores", vkWaitSemaphores},
    {"vkWaitSemaphoresKHR", vkWaitSemaphoresKHR},
    {"vkWriteAccelerationStructuresPropertiesKHR", vkWriteAccelerationStructuresPropertiesKHR},
    {"vkWriteMicromapsPropertiesEXT", vkWriteMicromapsPropertiesEXT},
};

static const struct vulkan_func vk_phys_dev_dispatch_table[] =
{
    {"vkCreateDevice", vkCreateDevice},
    {"vkEnumerateDeviceExtensionProperties", vkEnumerateDeviceExtensionProperties},
    {"vkEnumerateDeviceLayerProperties", vkEnumerateDeviceLayerProperties},
    {"vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR", vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR},
    {"vkGetPhysicalDeviceCalibrateableTimeDomainsEXT", vkGetPhysicalDeviceCalibrateableTimeDomainsEXT},
    {"vkGetPhysicalDeviceCooperativeMatrixPropertiesNV", vkGetPhysicalDeviceCooperativeMatrixPropertiesNV},
    {"vkGetPhysicalDeviceExternalBufferProperties", vkGetPhysicalDeviceExternalBufferProperties},
    {"vkGetPhysicalDeviceExternalBufferPropertiesKHR", vkGetPhysicalDeviceExternalBufferPropertiesKHR},
    {"vkGetPhysicalDeviceExternalFenceProperties", vkGetPhysicalDeviceExternalFenceProperties},
    {"vkGetPhysicalDeviceExternalFencePropertiesKHR", vkGetPhysicalDeviceExternalFencePropertiesKHR},
    {"vkGetPhysicalDeviceExternalSemaphoreProperties", vkGetPhysicalDeviceExternalSemaphoreProperties},
    {"vkGetPhysicalDeviceExternalSemaphorePropertiesKHR", vkGetPhysicalDeviceExternalSemaphorePropertiesKHR},
    {"vkGetPhysicalDeviceFeatures", vkGetPhysicalDeviceFeatures},
    {"vkGetPhysicalDeviceFeatures2", vkGetPhysicalDeviceFeatures2},
    {"vkGetPhysicalDeviceFeatures2KHR", vkGetPhysicalDeviceFeatures2KHR},
    {"vkGetPhysicalDeviceFormatProperties", vkGetPhysicalDeviceFormatProperties},
    {"vkGetPhysicalDeviceFormatProperties2", vkGetPhysicalDeviceFormatProperties2},
    {"vkGetPhysicalDeviceFormatProperties2KHR", vkGetPhysicalDeviceFormatProperties2KHR},
    {"vkGetPhysicalDeviceFragmentShadingRatesKHR", vkGetPhysicalDeviceFragmentShadingRatesKHR},
    {"vkGetPhysicalDeviceImageFormatProperties", vkGetPhysicalDeviceImageFormatProperties},
    {"vkGetPhysicalDeviceImageFormatProperties2", vkGetPhysicalDeviceImageFormatProperties2},
    {"vkGetPhysicalDeviceImageFormatProperties2KHR", vkGetPhysicalDeviceImageFormatProperties2KHR},
    {"vkGetPhysicalDeviceMemoryProperties", vkGetPhysicalDeviceMemoryProperties},
    {"vkGetPhysicalDeviceMemoryProperties2", vkGetPhysicalDeviceMemoryProperties2},
    {"vkGetPhysicalDeviceMemoryProperties2KHR", vkGetPhysicalDeviceMemoryProperties2KHR},
    {"vkGetPhysicalDeviceMultisamplePropertiesEXT", vkGetPhysicalDeviceMultisamplePropertiesEXT},
    {"vkGetPhysicalDeviceOpticalFlowImageFormatsNV", vkGetPhysicalDeviceOpticalFlowImageFormatsNV},
    {"vkGetPhysicalDevicePresentRectanglesKHR", vkGetPhysicalDevicePresentRectanglesKHR},
    {"vkGetPhysicalDeviceProperties", vkGetPhysicalDeviceProperties},
    {"vkGetPhysicalDeviceProperties2", vkGetPhysicalDeviceProperties2},
    {"vkGetPhysicalDeviceProperties2KHR", vkGetPhysicalDeviceProperties2KHR},
    {"vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR", vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR},
    {"vkGetPhysicalDeviceQueueFamilyProperties", vkGetPhysicalDeviceQueueFamilyProperties},
    {"vkGetPhysicalDeviceQueueFamilyProperties2", vkGetPhysicalDeviceQueueFamilyProperties2},
    {"vkGetPhysicalDeviceQueueFamilyProperties2KHR", vkGetPhysicalDeviceQueueFamilyProperties2KHR},
    {"vkGetPhysicalDeviceSparseImageFormatProperties", vkGetPhysicalDeviceSparseImageFormatProperties},
    {"vkGetPhysicalDeviceSparseImageFormatProperties2", vkGetPhysicalDeviceSparseImageFormatProperties2},
    {"vkGetPhysicalDeviceSparseImageFormatProperties2KHR", vkGetPhysicalDeviceSparseImageFormatProperties2KHR},
    {"vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV", vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV},
    {"vkGetPhysicalDeviceSurfaceCapabilities2KHR", vkGetPhysicalDeviceSurfaceCapabilities2KHR},
    {"vkGetPhysicalDeviceSurfaceCapabilitiesKHR", vkGetPhysicalDeviceSurfaceCapabilitiesKHR},
    {"vkGetPhysicalDeviceSurfaceFormats2KHR", vkGetPhysicalDeviceSurfaceFormats2KHR},
    {"vkGetPhysicalDeviceSurfaceFormatsKHR", vkGetPhysicalDeviceSurfaceFormatsKHR},
    {"vkGetPhysicalDeviceSurfacePresentModesKHR", vkGetPhysicalDeviceSurfacePresentModesKHR},
    {"vkGetPhysicalDeviceSurfaceSupportKHR", vkGetPhysicalDeviceSurfaceSupportKHR},
    {"vkGetPhysicalDeviceToolProperties", vkGetPhysicalDeviceToolProperties},
    {"vkGetPhysicalDeviceToolPropertiesEXT", vkGetPhysicalDeviceToolPropertiesEXT},
    {"vkGetPhysicalDeviceWin32PresentationSupportKHR", vkGetPhysicalDeviceWin32PresentationSupportKHR},
};

static const struct vulkan_func vk_instance_dispatch_table[] =
{
    {"vkCreateDebugReportCallbackEXT", vkCreateDebugReportCallbackEXT},
    {"vkCreateDebugUtilsMessengerEXT", vkCreateDebugUtilsMessengerEXT},
    {"vkCreateWin32SurfaceKHR", vkCreateWin32SurfaceKHR},
    {"vkDebugReportMessageEXT", vkDebugReportMessageEXT},
    {"vkDestroyDebugReportCallbackEXT", vkDestroyDebugReportCallbackEXT},
    {"vkDestroyDebugUtilsMessengerEXT", vkDestroyDebugUtilsMessengerEXT},
    {"vkDestroyInstance", vkDestroyInstance},
    {"vkDestroySurfaceKHR", vkDestroySurfaceKHR},
    {"vkEnumeratePhysicalDeviceGroups", vkEnumeratePhysicalDeviceGroups},
    {"vkEnumeratePhysicalDeviceGroupsKHR", vkEnumeratePhysicalDeviceGroupsKHR},
    {"vkEnumeratePhysicalDevices", vkEnumeratePhysicalDevices},
    {"vkSubmitDebugUtilsMessageEXT", vkSubmitDebugUtilsMessageEXT},
};

void *wine_vk_get_device_proc_addr(const char *name)
{
    unsigned int i;
    for (i = 0; i < ARRAY_SIZE(vk_device_dispatch_table); i++)
    {
        if (strcmp(vk_device_dispatch_table[i].name, name) == 0)
        {
            TRACE("Found name=%s in device table\n", debugstr_a(name));
            return vk_device_dispatch_table[i].func;
        }
    }
    return NULL;
}

void *wine_vk_get_phys_dev_proc_addr(const char *name)
{
    unsigned int i;
    for (i = 0; i < ARRAY_SIZE(vk_phys_dev_dispatch_table); i++)
    {
        if (strcmp(vk_phys_dev_dispatch_table[i].name, name) == 0)
        {
            TRACE("Found name=%s in physical device table\n", debugstr_a(name));
            return vk_phys_dev_dispatch_table[i].func;
        }
    }
    return NULL;
}

void *wine_vk_get_instance_proc_addr(const char *name)
{
    unsigned int i;
    for (i = 0; i < ARRAY_SIZE(vk_instance_dispatch_table); i++)
    {
        if (strcmp(vk_instance_dispatch_table[i].name, name) == 0)
        {
            TRACE("Found name=%s in instance table\n", debugstr_a(name));
            return vk_instance_dispatch_table[i].func;
        }
    }
    return NULL;
}
