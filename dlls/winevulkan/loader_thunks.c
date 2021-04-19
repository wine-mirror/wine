/* Automatically generated from Vulkan vk.xml; DO NOT EDIT!
 *
 * This file is generated from Vulkan vk.xml file covered
 * by the following copyright and permission notice:
 *
 * Copyright 2015-2021 The Khronos Group Inc.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "vulkan_loader.h"

WINE_DEFAULT_DEBUG_CHANNEL(vulkan);

VkResult WINAPI wine_vkAcquireNextImage2KHR(VkDevice device, const VkAcquireNextImageInfoKHR *pAcquireInfo, uint32_t *pImageIndex)
{
    return unix_funcs->p_vkAcquireNextImage2KHR(device, pAcquireInfo, pImageIndex);
}

VkResult WINAPI wine_vkAcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t *pImageIndex)
{
    return unix_funcs->p_vkAcquireNextImageKHR(device, swapchain, timeout, semaphore, fence, pImageIndex);
}

VkResult WINAPI wine_vkAcquirePerformanceConfigurationINTEL(VkDevice device, const VkPerformanceConfigurationAcquireInfoINTEL *pAcquireInfo, VkPerformanceConfigurationINTEL *pConfiguration)
{
    return unix_funcs->p_vkAcquirePerformanceConfigurationINTEL(device, pAcquireInfo, pConfiguration);
}

VkResult WINAPI wine_vkAcquireProfilingLockKHR(VkDevice device, const VkAcquireProfilingLockInfoKHR *pInfo)
{
    return unix_funcs->p_vkAcquireProfilingLockKHR(device, pInfo);
}

VkResult WINAPI wine_vkAllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo *pAllocateInfo, VkCommandBuffer *pCommandBuffers)
{
    return unix_funcs->p_vkAllocateCommandBuffers(device, pAllocateInfo, pCommandBuffers);
}

VkResult WINAPI wine_vkAllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo *pAllocateInfo, VkDescriptorSet *pDescriptorSets)
{
    return unix_funcs->p_vkAllocateDescriptorSets(device, pAllocateInfo, pDescriptorSets);
}

VkResult WINAPI wine_vkAllocateMemory(VkDevice device, const VkMemoryAllocateInfo *pAllocateInfo, const VkAllocationCallbacks *pAllocator, VkDeviceMemory *pMemory)
{
    return unix_funcs->p_vkAllocateMemory(device, pAllocateInfo, pAllocator, pMemory);
}

VkResult WINAPI wine_vkBeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo *pBeginInfo)
{
    return unix_funcs->p_vkBeginCommandBuffer(commandBuffer, pBeginInfo);
}

VkResult WINAPI wine_vkBindAccelerationStructureMemoryNV(VkDevice device, uint32_t bindInfoCount, const VkBindAccelerationStructureMemoryInfoNV *pBindInfos)
{
    return unix_funcs->p_vkBindAccelerationStructureMemoryNV(device, bindInfoCount, pBindInfos);
}

VkResult WINAPI wine_vkBindBufferMemory(VkDevice device, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset)
{
    return unix_funcs->p_vkBindBufferMemory(device, buffer, memory, memoryOffset);
}

VkResult WINAPI wine_vkBindBufferMemory2(VkDevice device, uint32_t bindInfoCount, const VkBindBufferMemoryInfo *pBindInfos)
{
    return unix_funcs->p_vkBindBufferMemory2(device, bindInfoCount, pBindInfos);
}

VkResult WINAPI wine_vkBindBufferMemory2KHR(VkDevice device, uint32_t bindInfoCount, const VkBindBufferMemoryInfo *pBindInfos)
{
    return unix_funcs->p_vkBindBufferMemory2KHR(device, bindInfoCount, pBindInfos);
}

VkResult WINAPI wine_vkBindImageMemory(VkDevice device, VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset)
{
    return unix_funcs->p_vkBindImageMemory(device, image, memory, memoryOffset);
}

VkResult WINAPI wine_vkBindImageMemory2(VkDevice device, uint32_t bindInfoCount, const VkBindImageMemoryInfo *pBindInfos)
{
    return unix_funcs->p_vkBindImageMemory2(device, bindInfoCount, pBindInfos);
}

VkResult WINAPI wine_vkBindImageMemory2KHR(VkDevice device, uint32_t bindInfoCount, const VkBindImageMemoryInfo *pBindInfos)
{
    return unix_funcs->p_vkBindImageMemory2KHR(device, bindInfoCount, pBindInfos);
}

VkResult WINAPI wine_vkBuildAccelerationStructuresKHR(VkDevice device, VkDeferredOperationKHR deferredOperation, uint32_t infoCount, const VkAccelerationStructureBuildGeometryInfoKHR *pInfos, const VkAccelerationStructureBuildRangeInfoKHR * const*ppBuildRangeInfos)
{
    return unix_funcs->p_vkBuildAccelerationStructuresKHR(device, deferredOperation, infoCount, pInfos, ppBuildRangeInfos);
}

void WINAPI wine_vkCmdBeginConditionalRenderingEXT(VkCommandBuffer commandBuffer, const VkConditionalRenderingBeginInfoEXT *pConditionalRenderingBegin)
{
    unix_funcs->p_vkCmdBeginConditionalRenderingEXT(commandBuffer, pConditionalRenderingBegin);
}

void WINAPI wine_vkCmdBeginDebugUtilsLabelEXT(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT *pLabelInfo)
{
    unix_funcs->p_vkCmdBeginDebugUtilsLabelEXT(commandBuffer, pLabelInfo);
}

void WINAPI wine_vkCmdBeginQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags)
{
    unix_funcs->p_vkCmdBeginQuery(commandBuffer, queryPool, query, flags);
}

void WINAPI wine_vkCmdBeginQueryIndexedEXT(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags, uint32_t index)
{
    unix_funcs->p_vkCmdBeginQueryIndexedEXT(commandBuffer, queryPool, query, flags, index);
}

void WINAPI wine_vkCmdBeginRenderPass(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo *pRenderPassBegin, VkSubpassContents contents)
{
    unix_funcs->p_vkCmdBeginRenderPass(commandBuffer, pRenderPassBegin, contents);
}

void WINAPI wine_vkCmdBeginRenderPass2(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo *pRenderPassBegin, const VkSubpassBeginInfo *pSubpassBeginInfo)
{
    unix_funcs->p_vkCmdBeginRenderPass2(commandBuffer, pRenderPassBegin, pSubpassBeginInfo);
}

void WINAPI wine_vkCmdBeginRenderPass2KHR(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo *pRenderPassBegin, const VkSubpassBeginInfo *pSubpassBeginInfo)
{
    unix_funcs->p_vkCmdBeginRenderPass2KHR(commandBuffer, pRenderPassBegin, pSubpassBeginInfo);
}

void WINAPI wine_vkCmdBeginTransformFeedbackEXT(VkCommandBuffer commandBuffer, uint32_t firstCounterBuffer, uint32_t counterBufferCount, const VkBuffer *pCounterBuffers, const VkDeviceSize *pCounterBufferOffsets)
{
    unix_funcs->p_vkCmdBeginTransformFeedbackEXT(commandBuffer, firstCounterBuffer, counterBufferCount, pCounterBuffers, pCounterBufferOffsets);
}

void WINAPI wine_vkCmdBindDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount, const VkDescriptorSet *pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t *pDynamicOffsets)
{
    unix_funcs->p_vkCmdBindDescriptorSets(commandBuffer, pipelineBindPoint, layout, firstSet, descriptorSetCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
}

void WINAPI wine_vkCmdBindIndexBuffer(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType)
{
    unix_funcs->p_vkCmdBindIndexBuffer(commandBuffer, buffer, offset, indexType);
}

void WINAPI wine_vkCmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline)
{
    unix_funcs->p_vkCmdBindPipeline(commandBuffer, pipelineBindPoint, pipeline);
}

void WINAPI wine_vkCmdBindPipelineShaderGroupNV(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline, uint32_t groupIndex)
{
    unix_funcs->p_vkCmdBindPipelineShaderGroupNV(commandBuffer, pipelineBindPoint, pipeline, groupIndex);
}

void WINAPI wine_vkCmdBindShadingRateImageNV(VkCommandBuffer commandBuffer, VkImageView imageView, VkImageLayout imageLayout)
{
    unix_funcs->p_vkCmdBindShadingRateImageNV(commandBuffer, imageView, imageLayout);
}

void WINAPI wine_vkCmdBindTransformFeedbackBuffersEXT(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer *pBuffers, const VkDeviceSize *pOffsets, const VkDeviceSize *pSizes)
{
    unix_funcs->p_vkCmdBindTransformFeedbackBuffersEXT(commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets, pSizes);
}

void WINAPI wine_vkCmdBindVertexBuffers(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer *pBuffers, const VkDeviceSize *pOffsets)
{
    unix_funcs->p_vkCmdBindVertexBuffers(commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets);
}

void WINAPI wine_vkCmdBindVertexBuffers2EXT(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer *pBuffers, const VkDeviceSize *pOffsets, const VkDeviceSize *pSizes, const VkDeviceSize *pStrides)
{
    unix_funcs->p_vkCmdBindVertexBuffers2EXT(commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets, pSizes, pStrides);
}

void WINAPI wine_vkCmdBlitImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit *pRegions, VkFilter filter)
{
    unix_funcs->p_vkCmdBlitImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions, filter);
}

void WINAPI wine_vkCmdBlitImage2KHR(VkCommandBuffer commandBuffer, const VkBlitImageInfo2KHR *pBlitImageInfo)
{
    unix_funcs->p_vkCmdBlitImage2KHR(commandBuffer, pBlitImageInfo);
}

void WINAPI wine_vkCmdBuildAccelerationStructureNV(VkCommandBuffer commandBuffer, const VkAccelerationStructureInfoNV *pInfo, VkBuffer instanceData, VkDeviceSize instanceOffset, VkBool32 update, VkAccelerationStructureNV dst, VkAccelerationStructureNV src, VkBuffer scratch, VkDeviceSize scratchOffset)
{
    unix_funcs->p_vkCmdBuildAccelerationStructureNV(commandBuffer, pInfo, instanceData, instanceOffset, update, dst, src, scratch, scratchOffset);
}

void WINAPI wine_vkCmdBuildAccelerationStructuresIndirectKHR(VkCommandBuffer commandBuffer, uint32_t infoCount, const VkAccelerationStructureBuildGeometryInfoKHR *pInfos, const VkDeviceAddress *pIndirectDeviceAddresses, const uint32_t *pIndirectStrides, const uint32_t * const*ppMaxPrimitiveCounts)
{
    unix_funcs->p_vkCmdBuildAccelerationStructuresIndirectKHR(commandBuffer, infoCount, pInfos, pIndirectDeviceAddresses, pIndirectStrides, ppMaxPrimitiveCounts);
}

void WINAPI wine_vkCmdBuildAccelerationStructuresKHR(VkCommandBuffer commandBuffer, uint32_t infoCount, const VkAccelerationStructureBuildGeometryInfoKHR *pInfos, const VkAccelerationStructureBuildRangeInfoKHR * const*ppBuildRangeInfos)
{
    unix_funcs->p_vkCmdBuildAccelerationStructuresKHR(commandBuffer, infoCount, pInfos, ppBuildRangeInfos);
}

void WINAPI wine_vkCmdClearAttachments(VkCommandBuffer commandBuffer, uint32_t attachmentCount, const VkClearAttachment *pAttachments, uint32_t rectCount, const VkClearRect *pRects)
{
    unix_funcs->p_vkCmdClearAttachments(commandBuffer, attachmentCount, pAttachments, rectCount, pRects);
}

void WINAPI wine_vkCmdClearColorImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, const VkClearColorValue *pColor, uint32_t rangeCount, const VkImageSubresourceRange *pRanges)
{
    unix_funcs->p_vkCmdClearColorImage(commandBuffer, image, imageLayout, pColor, rangeCount, pRanges);
}

void WINAPI wine_vkCmdClearDepthStencilImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, const VkClearDepthStencilValue *pDepthStencil, uint32_t rangeCount, const VkImageSubresourceRange *pRanges)
{
    unix_funcs->p_vkCmdClearDepthStencilImage(commandBuffer, image, imageLayout, pDepthStencil, rangeCount, pRanges);
}

void WINAPI wine_vkCmdCopyAccelerationStructureKHR(VkCommandBuffer commandBuffer, const VkCopyAccelerationStructureInfoKHR *pInfo)
{
    unix_funcs->p_vkCmdCopyAccelerationStructureKHR(commandBuffer, pInfo);
}

void WINAPI wine_vkCmdCopyAccelerationStructureNV(VkCommandBuffer commandBuffer, VkAccelerationStructureNV dst, VkAccelerationStructureNV src, VkCopyAccelerationStructureModeKHR mode)
{
    unix_funcs->p_vkCmdCopyAccelerationStructureNV(commandBuffer, dst, src, mode);
}

void WINAPI wine_vkCmdCopyAccelerationStructureToMemoryKHR(VkCommandBuffer commandBuffer, const VkCopyAccelerationStructureToMemoryInfoKHR *pInfo)
{
    unix_funcs->p_vkCmdCopyAccelerationStructureToMemoryKHR(commandBuffer, pInfo);
}

void WINAPI wine_vkCmdCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy *pRegions)
{
    unix_funcs->p_vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, regionCount, pRegions);
}

void WINAPI wine_vkCmdCopyBuffer2KHR(VkCommandBuffer commandBuffer, const VkCopyBufferInfo2KHR *pCopyBufferInfo)
{
    unix_funcs->p_vkCmdCopyBuffer2KHR(commandBuffer, pCopyBufferInfo);
}

void WINAPI wine_vkCmdCopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy *pRegions)
{
    unix_funcs->p_vkCmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions);
}

void WINAPI wine_vkCmdCopyBufferToImage2KHR(VkCommandBuffer commandBuffer, const VkCopyBufferToImageInfo2KHR *pCopyBufferToImageInfo)
{
    unix_funcs->p_vkCmdCopyBufferToImage2KHR(commandBuffer, pCopyBufferToImageInfo);
}

void WINAPI wine_vkCmdCopyImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageCopy *pRegions)
{
    unix_funcs->p_vkCmdCopyImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
}

void WINAPI wine_vkCmdCopyImage2KHR(VkCommandBuffer commandBuffer, const VkCopyImageInfo2KHR *pCopyImageInfo)
{
    unix_funcs->p_vkCmdCopyImage2KHR(commandBuffer, pCopyImageInfo);
}

void WINAPI wine_vkCmdCopyImageToBuffer(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferImageCopy *pRegions)
{
    unix_funcs->p_vkCmdCopyImageToBuffer(commandBuffer, srcImage, srcImageLayout, dstBuffer, regionCount, pRegions);
}

void WINAPI wine_vkCmdCopyImageToBuffer2KHR(VkCommandBuffer commandBuffer, const VkCopyImageToBufferInfo2KHR *pCopyImageToBufferInfo)
{
    unix_funcs->p_vkCmdCopyImageToBuffer2KHR(commandBuffer, pCopyImageToBufferInfo);
}

void WINAPI wine_vkCmdCopyMemoryToAccelerationStructureKHR(VkCommandBuffer commandBuffer, const VkCopyMemoryToAccelerationStructureInfoKHR *pInfo)
{
    unix_funcs->p_vkCmdCopyMemoryToAccelerationStructureKHR(commandBuffer, pInfo);
}

void WINAPI wine_vkCmdCopyQueryPoolResults(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize stride, VkQueryResultFlags flags)
{
    unix_funcs->p_vkCmdCopyQueryPoolResults(commandBuffer, queryPool, firstQuery, queryCount, dstBuffer, dstOffset, stride, flags);
}

void WINAPI wine_vkCmdDebugMarkerBeginEXT(VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT *pMarkerInfo)
{
    unix_funcs->p_vkCmdDebugMarkerBeginEXT(commandBuffer, pMarkerInfo);
}

void WINAPI wine_vkCmdDebugMarkerEndEXT(VkCommandBuffer commandBuffer)
{
    unix_funcs->p_vkCmdDebugMarkerEndEXT(commandBuffer);
}

void WINAPI wine_vkCmdDebugMarkerInsertEXT(VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT *pMarkerInfo)
{
    unix_funcs->p_vkCmdDebugMarkerInsertEXT(commandBuffer, pMarkerInfo);
}

void WINAPI wine_vkCmdDispatch(VkCommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    unix_funcs->p_vkCmdDispatch(commandBuffer, groupCountX, groupCountY, groupCountZ);
}

void WINAPI wine_vkCmdDispatchBase(VkCommandBuffer commandBuffer, uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    unix_funcs->p_vkCmdDispatchBase(commandBuffer, baseGroupX, baseGroupY, baseGroupZ, groupCountX, groupCountY, groupCountZ);
}

void WINAPI wine_vkCmdDispatchBaseKHR(VkCommandBuffer commandBuffer, uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    unix_funcs->p_vkCmdDispatchBaseKHR(commandBuffer, baseGroupX, baseGroupY, baseGroupZ, groupCountX, groupCountY, groupCountZ);
}

void WINAPI wine_vkCmdDispatchIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset)
{
    unix_funcs->p_vkCmdDispatchIndirect(commandBuffer, buffer, offset);
}

void WINAPI wine_vkCmdDraw(VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    unix_funcs->p_vkCmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void WINAPI wine_vkCmdDrawIndexed(VkCommandBuffer commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
{
    unix_funcs->p_vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void WINAPI wine_vkCmdDrawIndexedIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
{
    unix_funcs->p_vkCmdDrawIndexedIndirect(commandBuffer, buffer, offset, drawCount, stride);
}

void WINAPI wine_vkCmdDrawIndexedIndirectCount(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
    unix_funcs->p_vkCmdDrawIndexedIndirectCount(commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}

void WINAPI wine_vkCmdDrawIndexedIndirectCountAMD(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
    unix_funcs->p_vkCmdDrawIndexedIndirectCountAMD(commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}

void WINAPI wine_vkCmdDrawIndexedIndirectCountKHR(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
    unix_funcs->p_vkCmdDrawIndexedIndirectCountKHR(commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}

void WINAPI wine_vkCmdDrawIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
{
    unix_funcs->p_vkCmdDrawIndirect(commandBuffer, buffer, offset, drawCount, stride);
}

void WINAPI wine_vkCmdDrawIndirectByteCountEXT(VkCommandBuffer commandBuffer, uint32_t instanceCount, uint32_t firstInstance, VkBuffer counterBuffer, VkDeviceSize counterBufferOffset, uint32_t counterOffset, uint32_t vertexStride)
{
    unix_funcs->p_vkCmdDrawIndirectByteCountEXT(commandBuffer, instanceCount, firstInstance, counterBuffer, counterBufferOffset, counterOffset, vertexStride);
}

void WINAPI wine_vkCmdDrawIndirectCount(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
    unix_funcs->p_vkCmdDrawIndirectCount(commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}

void WINAPI wine_vkCmdDrawIndirectCountAMD(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
    unix_funcs->p_vkCmdDrawIndirectCountAMD(commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}

void WINAPI wine_vkCmdDrawIndirectCountKHR(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
    unix_funcs->p_vkCmdDrawIndirectCountKHR(commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}

void WINAPI wine_vkCmdDrawMeshTasksIndirectCountNV(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
    unix_funcs->p_vkCmdDrawMeshTasksIndirectCountNV(commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}

void WINAPI wine_vkCmdDrawMeshTasksIndirectNV(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
{
    unix_funcs->p_vkCmdDrawMeshTasksIndirectNV(commandBuffer, buffer, offset, drawCount, stride);
}

void WINAPI wine_vkCmdDrawMeshTasksNV(VkCommandBuffer commandBuffer, uint32_t taskCount, uint32_t firstTask)
{
    unix_funcs->p_vkCmdDrawMeshTasksNV(commandBuffer, taskCount, firstTask);
}

void WINAPI wine_vkCmdEndConditionalRenderingEXT(VkCommandBuffer commandBuffer)
{
    unix_funcs->p_vkCmdEndConditionalRenderingEXT(commandBuffer);
}

void WINAPI wine_vkCmdEndDebugUtilsLabelEXT(VkCommandBuffer commandBuffer)
{
    unix_funcs->p_vkCmdEndDebugUtilsLabelEXT(commandBuffer);
}

void WINAPI wine_vkCmdEndQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query)
{
    unix_funcs->p_vkCmdEndQuery(commandBuffer, queryPool, query);
}

void WINAPI wine_vkCmdEndQueryIndexedEXT(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query, uint32_t index)
{
    unix_funcs->p_vkCmdEndQueryIndexedEXT(commandBuffer, queryPool, query, index);
}

void WINAPI wine_vkCmdEndRenderPass(VkCommandBuffer commandBuffer)
{
    unix_funcs->p_vkCmdEndRenderPass(commandBuffer);
}

void WINAPI wine_vkCmdEndRenderPass2(VkCommandBuffer commandBuffer, const VkSubpassEndInfo *pSubpassEndInfo)
{
    unix_funcs->p_vkCmdEndRenderPass2(commandBuffer, pSubpassEndInfo);
}

void WINAPI wine_vkCmdEndRenderPass2KHR(VkCommandBuffer commandBuffer, const VkSubpassEndInfo *pSubpassEndInfo)
{
    unix_funcs->p_vkCmdEndRenderPass2KHR(commandBuffer, pSubpassEndInfo);
}

void WINAPI wine_vkCmdEndTransformFeedbackEXT(VkCommandBuffer commandBuffer, uint32_t firstCounterBuffer, uint32_t counterBufferCount, const VkBuffer *pCounterBuffers, const VkDeviceSize *pCounterBufferOffsets)
{
    unix_funcs->p_vkCmdEndTransformFeedbackEXT(commandBuffer, firstCounterBuffer, counterBufferCount, pCounterBuffers, pCounterBufferOffsets);
}

void WINAPI wine_vkCmdExecuteCommands(VkCommandBuffer commandBuffer, uint32_t commandBufferCount, const VkCommandBuffer *pCommandBuffers)
{
    unix_funcs->p_vkCmdExecuteCommands(commandBuffer, commandBufferCount, pCommandBuffers);
}

void WINAPI wine_vkCmdExecuteGeneratedCommandsNV(VkCommandBuffer commandBuffer, VkBool32 isPreprocessed, const VkGeneratedCommandsInfoNV *pGeneratedCommandsInfo)
{
    unix_funcs->p_vkCmdExecuteGeneratedCommandsNV(commandBuffer, isPreprocessed, pGeneratedCommandsInfo);
}

void WINAPI wine_vkCmdFillBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data)
{
    unix_funcs->p_vkCmdFillBuffer(commandBuffer, dstBuffer, dstOffset, size, data);
}

void WINAPI wine_vkCmdInsertDebugUtilsLabelEXT(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT *pLabelInfo)
{
    unix_funcs->p_vkCmdInsertDebugUtilsLabelEXT(commandBuffer, pLabelInfo);
}

void WINAPI wine_vkCmdNextSubpass(VkCommandBuffer commandBuffer, VkSubpassContents contents)
{
    unix_funcs->p_vkCmdNextSubpass(commandBuffer, contents);
}

void WINAPI wine_vkCmdNextSubpass2(VkCommandBuffer commandBuffer, const VkSubpassBeginInfo *pSubpassBeginInfo, const VkSubpassEndInfo *pSubpassEndInfo)
{
    unix_funcs->p_vkCmdNextSubpass2(commandBuffer, pSubpassBeginInfo, pSubpassEndInfo);
}

void WINAPI wine_vkCmdNextSubpass2KHR(VkCommandBuffer commandBuffer, const VkSubpassBeginInfo *pSubpassBeginInfo, const VkSubpassEndInfo *pSubpassEndInfo)
{
    unix_funcs->p_vkCmdNextSubpass2KHR(commandBuffer, pSubpassBeginInfo, pSubpassEndInfo);
}

void WINAPI wine_vkCmdPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier *pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier *pImageMemoryBarriers)
{
    unix_funcs->p_vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
}

void WINAPI wine_vkCmdPipelineBarrier2KHR(VkCommandBuffer commandBuffer, const VkDependencyInfoKHR *pDependencyInfo)
{
    unix_funcs->p_vkCmdPipelineBarrier2KHR(commandBuffer, pDependencyInfo);
}

void WINAPI wine_vkCmdPreprocessGeneratedCommandsNV(VkCommandBuffer commandBuffer, const VkGeneratedCommandsInfoNV *pGeneratedCommandsInfo)
{
    unix_funcs->p_vkCmdPreprocessGeneratedCommandsNV(commandBuffer, pGeneratedCommandsInfo);
}

void WINAPI wine_vkCmdPushConstants(VkCommandBuffer commandBuffer, VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void *pValues)
{
    unix_funcs->p_vkCmdPushConstants(commandBuffer, layout, stageFlags, offset, size, pValues);
}

void WINAPI wine_vkCmdPushDescriptorSetKHR(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t set, uint32_t descriptorWriteCount, const VkWriteDescriptorSet *pDescriptorWrites)
{
    unix_funcs->p_vkCmdPushDescriptorSetKHR(commandBuffer, pipelineBindPoint, layout, set, descriptorWriteCount, pDescriptorWrites);
}

void WINAPI wine_vkCmdPushDescriptorSetWithTemplateKHR(VkCommandBuffer commandBuffer, VkDescriptorUpdateTemplate descriptorUpdateTemplate, VkPipelineLayout layout, uint32_t set, const void *pData)
{
    unix_funcs->p_vkCmdPushDescriptorSetWithTemplateKHR(commandBuffer, descriptorUpdateTemplate, layout, set, pData);
}

void WINAPI wine_vkCmdResetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask)
{
    unix_funcs->p_vkCmdResetEvent(commandBuffer, event, stageMask);
}

void WINAPI wine_vkCmdResetEvent2KHR(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags2KHR stageMask)
{
    unix_funcs->p_vkCmdResetEvent2KHR(commandBuffer, event, stageMask);
}

void WINAPI wine_vkCmdResetQueryPool(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount)
{
    unix_funcs->p_vkCmdResetQueryPool(commandBuffer, queryPool, firstQuery, queryCount);
}

void WINAPI wine_vkCmdResolveImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageResolve *pRegions)
{
    unix_funcs->p_vkCmdResolveImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
}

void WINAPI wine_vkCmdResolveImage2KHR(VkCommandBuffer commandBuffer, const VkResolveImageInfo2KHR *pResolveImageInfo)
{
    unix_funcs->p_vkCmdResolveImage2KHR(commandBuffer, pResolveImageInfo);
}

void WINAPI wine_vkCmdSetBlendConstants(VkCommandBuffer commandBuffer, const float blendConstants[4])
{
    unix_funcs->p_vkCmdSetBlendConstants(commandBuffer, blendConstants);
}

void WINAPI wine_vkCmdSetCheckpointNV(VkCommandBuffer commandBuffer, const void *pCheckpointMarker)
{
    unix_funcs->p_vkCmdSetCheckpointNV(commandBuffer, pCheckpointMarker);
}

void WINAPI wine_vkCmdSetCoarseSampleOrderNV(VkCommandBuffer commandBuffer, VkCoarseSampleOrderTypeNV sampleOrderType, uint32_t customSampleOrderCount, const VkCoarseSampleOrderCustomNV *pCustomSampleOrders)
{
    unix_funcs->p_vkCmdSetCoarseSampleOrderNV(commandBuffer, sampleOrderType, customSampleOrderCount, pCustomSampleOrders);
}

void WINAPI wine_vkCmdSetColorWriteEnableEXT(VkCommandBuffer commandBuffer, uint32_t attachmentCount, const VkBool32 *pColorWriteEnables)
{
    unix_funcs->p_vkCmdSetColorWriteEnableEXT(commandBuffer, attachmentCount, pColorWriteEnables);
}

void WINAPI wine_vkCmdSetCullModeEXT(VkCommandBuffer commandBuffer, VkCullModeFlags cullMode)
{
    unix_funcs->p_vkCmdSetCullModeEXT(commandBuffer, cullMode);
}

void WINAPI wine_vkCmdSetDepthBias(VkCommandBuffer commandBuffer, float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor)
{
    unix_funcs->p_vkCmdSetDepthBias(commandBuffer, depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
}

void WINAPI wine_vkCmdSetDepthBiasEnableEXT(VkCommandBuffer commandBuffer, VkBool32 depthBiasEnable)
{
    unix_funcs->p_vkCmdSetDepthBiasEnableEXT(commandBuffer, depthBiasEnable);
}

void WINAPI wine_vkCmdSetDepthBounds(VkCommandBuffer commandBuffer, float minDepthBounds, float maxDepthBounds)
{
    unix_funcs->p_vkCmdSetDepthBounds(commandBuffer, minDepthBounds, maxDepthBounds);
}

void WINAPI wine_vkCmdSetDepthBoundsTestEnableEXT(VkCommandBuffer commandBuffer, VkBool32 depthBoundsTestEnable)
{
    unix_funcs->p_vkCmdSetDepthBoundsTestEnableEXT(commandBuffer, depthBoundsTestEnable);
}

void WINAPI wine_vkCmdSetDepthCompareOpEXT(VkCommandBuffer commandBuffer, VkCompareOp depthCompareOp)
{
    unix_funcs->p_vkCmdSetDepthCompareOpEXT(commandBuffer, depthCompareOp);
}

void WINAPI wine_vkCmdSetDepthTestEnableEXT(VkCommandBuffer commandBuffer, VkBool32 depthTestEnable)
{
    unix_funcs->p_vkCmdSetDepthTestEnableEXT(commandBuffer, depthTestEnable);
}

void WINAPI wine_vkCmdSetDepthWriteEnableEXT(VkCommandBuffer commandBuffer, VkBool32 depthWriteEnable)
{
    unix_funcs->p_vkCmdSetDepthWriteEnableEXT(commandBuffer, depthWriteEnable);
}

void WINAPI wine_vkCmdSetDeviceMask(VkCommandBuffer commandBuffer, uint32_t deviceMask)
{
    unix_funcs->p_vkCmdSetDeviceMask(commandBuffer, deviceMask);
}

void WINAPI wine_vkCmdSetDeviceMaskKHR(VkCommandBuffer commandBuffer, uint32_t deviceMask)
{
    unix_funcs->p_vkCmdSetDeviceMaskKHR(commandBuffer, deviceMask);
}

void WINAPI wine_vkCmdSetDiscardRectangleEXT(VkCommandBuffer commandBuffer, uint32_t firstDiscardRectangle, uint32_t discardRectangleCount, const VkRect2D *pDiscardRectangles)
{
    unix_funcs->p_vkCmdSetDiscardRectangleEXT(commandBuffer, firstDiscardRectangle, discardRectangleCount, pDiscardRectangles);
}

void WINAPI wine_vkCmdSetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask)
{
    unix_funcs->p_vkCmdSetEvent(commandBuffer, event, stageMask);
}

void WINAPI wine_vkCmdSetEvent2KHR(VkCommandBuffer commandBuffer, VkEvent event, const VkDependencyInfoKHR *pDependencyInfo)
{
    unix_funcs->p_vkCmdSetEvent2KHR(commandBuffer, event, pDependencyInfo);
}

void WINAPI wine_vkCmdSetExclusiveScissorNV(VkCommandBuffer commandBuffer, uint32_t firstExclusiveScissor, uint32_t exclusiveScissorCount, const VkRect2D *pExclusiveScissors)
{
    unix_funcs->p_vkCmdSetExclusiveScissorNV(commandBuffer, firstExclusiveScissor, exclusiveScissorCount, pExclusiveScissors);
}

void WINAPI wine_vkCmdSetFragmentShadingRateEnumNV(VkCommandBuffer commandBuffer, VkFragmentShadingRateNV shadingRate, const VkFragmentShadingRateCombinerOpKHR combinerOps[2])
{
    unix_funcs->p_vkCmdSetFragmentShadingRateEnumNV(commandBuffer, shadingRate, combinerOps);
}

void WINAPI wine_vkCmdSetFragmentShadingRateKHR(VkCommandBuffer commandBuffer, const VkExtent2D *pFragmentSize, const VkFragmentShadingRateCombinerOpKHR combinerOps[2])
{
    unix_funcs->p_vkCmdSetFragmentShadingRateKHR(commandBuffer, pFragmentSize, combinerOps);
}

void WINAPI wine_vkCmdSetFrontFaceEXT(VkCommandBuffer commandBuffer, VkFrontFace frontFace)
{
    unix_funcs->p_vkCmdSetFrontFaceEXT(commandBuffer, frontFace);
}

void WINAPI wine_vkCmdSetLineStippleEXT(VkCommandBuffer commandBuffer, uint32_t lineStippleFactor, uint16_t lineStipplePattern)
{
    unix_funcs->p_vkCmdSetLineStippleEXT(commandBuffer, lineStippleFactor, lineStipplePattern);
}

void WINAPI wine_vkCmdSetLineWidth(VkCommandBuffer commandBuffer, float lineWidth)
{
    unix_funcs->p_vkCmdSetLineWidth(commandBuffer, lineWidth);
}

void WINAPI wine_vkCmdSetLogicOpEXT(VkCommandBuffer commandBuffer, VkLogicOp logicOp)
{
    unix_funcs->p_vkCmdSetLogicOpEXT(commandBuffer, logicOp);
}

void WINAPI wine_vkCmdSetPatchControlPointsEXT(VkCommandBuffer commandBuffer, uint32_t patchControlPoints)
{
    unix_funcs->p_vkCmdSetPatchControlPointsEXT(commandBuffer, patchControlPoints);
}

VkResult WINAPI wine_vkCmdSetPerformanceMarkerINTEL(VkCommandBuffer commandBuffer, const VkPerformanceMarkerInfoINTEL *pMarkerInfo)
{
    return unix_funcs->p_vkCmdSetPerformanceMarkerINTEL(commandBuffer, pMarkerInfo);
}

VkResult WINAPI wine_vkCmdSetPerformanceOverrideINTEL(VkCommandBuffer commandBuffer, const VkPerformanceOverrideInfoINTEL *pOverrideInfo)
{
    return unix_funcs->p_vkCmdSetPerformanceOverrideINTEL(commandBuffer, pOverrideInfo);
}

VkResult WINAPI wine_vkCmdSetPerformanceStreamMarkerINTEL(VkCommandBuffer commandBuffer, const VkPerformanceStreamMarkerInfoINTEL *pMarkerInfo)
{
    return unix_funcs->p_vkCmdSetPerformanceStreamMarkerINTEL(commandBuffer, pMarkerInfo);
}

void WINAPI wine_vkCmdSetPrimitiveRestartEnableEXT(VkCommandBuffer commandBuffer, VkBool32 primitiveRestartEnable)
{
    unix_funcs->p_vkCmdSetPrimitiveRestartEnableEXT(commandBuffer, primitiveRestartEnable);
}

void WINAPI wine_vkCmdSetPrimitiveTopologyEXT(VkCommandBuffer commandBuffer, VkPrimitiveTopology primitiveTopology)
{
    unix_funcs->p_vkCmdSetPrimitiveTopologyEXT(commandBuffer, primitiveTopology);
}

void WINAPI wine_vkCmdSetRasterizerDiscardEnableEXT(VkCommandBuffer commandBuffer, VkBool32 rasterizerDiscardEnable)
{
    unix_funcs->p_vkCmdSetRasterizerDiscardEnableEXT(commandBuffer, rasterizerDiscardEnable);
}

void WINAPI wine_vkCmdSetRayTracingPipelineStackSizeKHR(VkCommandBuffer commandBuffer, uint32_t pipelineStackSize)
{
    unix_funcs->p_vkCmdSetRayTracingPipelineStackSizeKHR(commandBuffer, pipelineStackSize);
}

void WINAPI wine_vkCmdSetSampleLocationsEXT(VkCommandBuffer commandBuffer, const VkSampleLocationsInfoEXT *pSampleLocationsInfo)
{
    unix_funcs->p_vkCmdSetSampleLocationsEXT(commandBuffer, pSampleLocationsInfo);
}

void WINAPI wine_vkCmdSetScissor(VkCommandBuffer commandBuffer, uint32_t firstScissor, uint32_t scissorCount, const VkRect2D *pScissors)
{
    unix_funcs->p_vkCmdSetScissor(commandBuffer, firstScissor, scissorCount, pScissors);
}

void WINAPI wine_vkCmdSetScissorWithCountEXT(VkCommandBuffer commandBuffer, uint32_t scissorCount, const VkRect2D *pScissors)
{
    unix_funcs->p_vkCmdSetScissorWithCountEXT(commandBuffer, scissorCount, pScissors);
}

void WINAPI wine_vkCmdSetStencilCompareMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t compareMask)
{
    unix_funcs->p_vkCmdSetStencilCompareMask(commandBuffer, faceMask, compareMask);
}

void WINAPI wine_vkCmdSetStencilOpEXT(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, VkStencilOp failOp, VkStencilOp passOp, VkStencilOp depthFailOp, VkCompareOp compareOp)
{
    unix_funcs->p_vkCmdSetStencilOpEXT(commandBuffer, faceMask, failOp, passOp, depthFailOp, compareOp);
}

void WINAPI wine_vkCmdSetStencilReference(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t reference)
{
    unix_funcs->p_vkCmdSetStencilReference(commandBuffer, faceMask, reference);
}

void WINAPI wine_vkCmdSetStencilTestEnableEXT(VkCommandBuffer commandBuffer, VkBool32 stencilTestEnable)
{
    unix_funcs->p_vkCmdSetStencilTestEnableEXT(commandBuffer, stencilTestEnable);
}

void WINAPI wine_vkCmdSetStencilWriteMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t writeMask)
{
    unix_funcs->p_vkCmdSetStencilWriteMask(commandBuffer, faceMask, writeMask);
}

void WINAPI wine_vkCmdSetVertexInputEXT(VkCommandBuffer commandBuffer, uint32_t vertexBindingDescriptionCount, const VkVertexInputBindingDescription2EXT *pVertexBindingDescriptions, uint32_t vertexAttributeDescriptionCount, const VkVertexInputAttributeDescription2EXT *pVertexAttributeDescriptions)
{
    unix_funcs->p_vkCmdSetVertexInputEXT(commandBuffer, vertexBindingDescriptionCount, pVertexBindingDescriptions, vertexAttributeDescriptionCount, pVertexAttributeDescriptions);
}

void WINAPI wine_vkCmdSetViewport(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkViewport *pViewports)
{
    unix_funcs->p_vkCmdSetViewport(commandBuffer, firstViewport, viewportCount, pViewports);
}

void WINAPI wine_vkCmdSetViewportShadingRatePaletteNV(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkShadingRatePaletteNV *pShadingRatePalettes)
{
    unix_funcs->p_vkCmdSetViewportShadingRatePaletteNV(commandBuffer, firstViewport, viewportCount, pShadingRatePalettes);
}

void WINAPI wine_vkCmdSetViewportWScalingNV(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkViewportWScalingNV *pViewportWScalings)
{
    unix_funcs->p_vkCmdSetViewportWScalingNV(commandBuffer, firstViewport, viewportCount, pViewportWScalings);
}

void WINAPI wine_vkCmdSetViewportWithCountEXT(VkCommandBuffer commandBuffer, uint32_t viewportCount, const VkViewport *pViewports)
{
    unix_funcs->p_vkCmdSetViewportWithCountEXT(commandBuffer, viewportCount, pViewports);
}

void WINAPI wine_vkCmdTraceRaysIndirectKHR(VkCommandBuffer commandBuffer, const VkStridedDeviceAddressRegionKHR *pRaygenShaderBindingTable, const VkStridedDeviceAddressRegionKHR *pMissShaderBindingTable, const VkStridedDeviceAddressRegionKHR *pHitShaderBindingTable, const VkStridedDeviceAddressRegionKHR *pCallableShaderBindingTable, VkDeviceAddress indirectDeviceAddress)
{
    unix_funcs->p_vkCmdTraceRaysIndirectKHR(commandBuffer, pRaygenShaderBindingTable, pMissShaderBindingTable, pHitShaderBindingTable, pCallableShaderBindingTable, indirectDeviceAddress);
}

void WINAPI wine_vkCmdTraceRaysKHR(VkCommandBuffer commandBuffer, const VkStridedDeviceAddressRegionKHR *pRaygenShaderBindingTable, const VkStridedDeviceAddressRegionKHR *pMissShaderBindingTable, const VkStridedDeviceAddressRegionKHR *pHitShaderBindingTable, const VkStridedDeviceAddressRegionKHR *pCallableShaderBindingTable, uint32_t width, uint32_t height, uint32_t depth)
{
    unix_funcs->p_vkCmdTraceRaysKHR(commandBuffer, pRaygenShaderBindingTable, pMissShaderBindingTable, pHitShaderBindingTable, pCallableShaderBindingTable, width, height, depth);
}

void WINAPI wine_vkCmdTraceRaysNV(VkCommandBuffer commandBuffer, VkBuffer raygenShaderBindingTableBuffer, VkDeviceSize raygenShaderBindingOffset, VkBuffer missShaderBindingTableBuffer, VkDeviceSize missShaderBindingOffset, VkDeviceSize missShaderBindingStride, VkBuffer hitShaderBindingTableBuffer, VkDeviceSize hitShaderBindingOffset, VkDeviceSize hitShaderBindingStride, VkBuffer callableShaderBindingTableBuffer, VkDeviceSize callableShaderBindingOffset, VkDeviceSize callableShaderBindingStride, uint32_t width, uint32_t height, uint32_t depth)
{
    unix_funcs->p_vkCmdTraceRaysNV(commandBuffer, raygenShaderBindingTableBuffer, raygenShaderBindingOffset, missShaderBindingTableBuffer, missShaderBindingOffset, missShaderBindingStride, hitShaderBindingTableBuffer, hitShaderBindingOffset, hitShaderBindingStride, callableShaderBindingTableBuffer, callableShaderBindingOffset, callableShaderBindingStride, width, height, depth);
}

void WINAPI wine_vkCmdUpdateBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void *pData)
{
    unix_funcs->p_vkCmdUpdateBuffer(commandBuffer, dstBuffer, dstOffset, dataSize, pData);
}

void WINAPI wine_vkCmdWaitEvents(VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent *pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier *pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier *pImageMemoryBarriers)
{
    unix_funcs->p_vkCmdWaitEvents(commandBuffer, eventCount, pEvents, srcStageMask, dstStageMask, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
}

void WINAPI wine_vkCmdWaitEvents2KHR(VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent *pEvents, const VkDependencyInfoKHR *pDependencyInfos)
{
    unix_funcs->p_vkCmdWaitEvents2KHR(commandBuffer, eventCount, pEvents, pDependencyInfos);
}

void WINAPI wine_vkCmdWriteAccelerationStructuresPropertiesKHR(VkCommandBuffer commandBuffer, uint32_t accelerationStructureCount, const VkAccelerationStructureKHR *pAccelerationStructures, VkQueryType queryType, VkQueryPool queryPool, uint32_t firstQuery)
{
    unix_funcs->p_vkCmdWriteAccelerationStructuresPropertiesKHR(commandBuffer, accelerationStructureCount, pAccelerationStructures, queryType, queryPool, firstQuery);
}

void WINAPI wine_vkCmdWriteAccelerationStructuresPropertiesNV(VkCommandBuffer commandBuffer, uint32_t accelerationStructureCount, const VkAccelerationStructureNV *pAccelerationStructures, VkQueryType queryType, VkQueryPool queryPool, uint32_t firstQuery)
{
    unix_funcs->p_vkCmdWriteAccelerationStructuresPropertiesNV(commandBuffer, accelerationStructureCount, pAccelerationStructures, queryType, queryPool, firstQuery);
}

void WINAPI wine_vkCmdWriteBufferMarker2AMD(VkCommandBuffer commandBuffer, VkPipelineStageFlags2KHR stage, VkBuffer dstBuffer, VkDeviceSize dstOffset, uint32_t marker)
{
    unix_funcs->p_vkCmdWriteBufferMarker2AMD(commandBuffer, stage, dstBuffer, dstOffset, marker);
}

void WINAPI wine_vkCmdWriteBufferMarkerAMD(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, VkBuffer dstBuffer, VkDeviceSize dstOffset, uint32_t marker)
{
    unix_funcs->p_vkCmdWriteBufferMarkerAMD(commandBuffer, pipelineStage, dstBuffer, dstOffset, marker);
}

void WINAPI wine_vkCmdWriteTimestamp(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool, uint32_t query)
{
    unix_funcs->p_vkCmdWriteTimestamp(commandBuffer, pipelineStage, queryPool, query);
}

void WINAPI wine_vkCmdWriteTimestamp2KHR(VkCommandBuffer commandBuffer, VkPipelineStageFlags2KHR stage, VkQueryPool queryPool, uint32_t query)
{
    unix_funcs->p_vkCmdWriteTimestamp2KHR(commandBuffer, stage, queryPool, query);
}

VkResult WINAPI wine_vkCompileDeferredNV(VkDevice device, VkPipeline pipeline, uint32_t shader)
{
    return unix_funcs->p_vkCompileDeferredNV(device, pipeline, shader);
}

VkResult WINAPI wine_vkCopyAccelerationStructureKHR(VkDevice device, VkDeferredOperationKHR deferredOperation, const VkCopyAccelerationStructureInfoKHR *pInfo)
{
    return unix_funcs->p_vkCopyAccelerationStructureKHR(device, deferredOperation, pInfo);
}

VkResult WINAPI wine_vkCopyAccelerationStructureToMemoryKHR(VkDevice device, VkDeferredOperationKHR deferredOperation, const VkCopyAccelerationStructureToMemoryInfoKHR *pInfo)
{
    return unix_funcs->p_vkCopyAccelerationStructureToMemoryKHR(device, deferredOperation, pInfo);
}

VkResult WINAPI wine_vkCopyMemoryToAccelerationStructureKHR(VkDevice device, VkDeferredOperationKHR deferredOperation, const VkCopyMemoryToAccelerationStructureInfoKHR *pInfo)
{
    return unix_funcs->p_vkCopyMemoryToAccelerationStructureKHR(device, deferredOperation, pInfo);
}

VkResult WINAPI wine_vkCreateAccelerationStructureKHR(VkDevice device, const VkAccelerationStructureCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkAccelerationStructureKHR *pAccelerationStructure)
{
    return unix_funcs->p_vkCreateAccelerationStructureKHR(device, pCreateInfo, pAllocator, pAccelerationStructure);
}

VkResult WINAPI wine_vkCreateAccelerationStructureNV(VkDevice device, const VkAccelerationStructureCreateInfoNV *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkAccelerationStructureNV *pAccelerationStructure)
{
    return unix_funcs->p_vkCreateAccelerationStructureNV(device, pCreateInfo, pAllocator, pAccelerationStructure);
}

VkResult WINAPI wine_vkCreateBuffer(VkDevice device, const VkBufferCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkBuffer *pBuffer)
{
    return unix_funcs->p_vkCreateBuffer(device, pCreateInfo, pAllocator, pBuffer);
}

VkResult WINAPI wine_vkCreateBufferView(VkDevice device, const VkBufferViewCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkBufferView *pView)
{
    return unix_funcs->p_vkCreateBufferView(device, pCreateInfo, pAllocator, pView);
}

VkResult WINAPI wine_vkCreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkCommandPool *pCommandPool)
{
    return unix_funcs->p_vkCreateCommandPool(device, pCreateInfo, pAllocator, pCommandPool);
}

VkResult WINAPI wine_vkCreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkComputePipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines)
{
    return unix_funcs->p_vkCreateComputePipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
}

VkResult WINAPI wine_vkCreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugReportCallbackEXT *pCallback)
{
    return unix_funcs->p_vkCreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pCallback);
}

VkResult WINAPI wine_vkCreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pMessenger)
{
    return unix_funcs->p_vkCreateDebugUtilsMessengerEXT(instance, pCreateInfo, pAllocator, pMessenger);
}

VkResult WINAPI wine_vkCreateDeferredOperationKHR(VkDevice device, const VkAllocationCallbacks *pAllocator, VkDeferredOperationKHR *pDeferredOperation)
{
    return unix_funcs->p_vkCreateDeferredOperationKHR(device, pAllocator, pDeferredOperation);
}

VkResult WINAPI wine_vkCreateDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDescriptorPool *pDescriptorPool)
{
    return unix_funcs->p_vkCreateDescriptorPool(device, pCreateInfo, pAllocator, pDescriptorPool);
}

VkResult WINAPI wine_vkCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDescriptorSetLayout *pSetLayout)
{
    return unix_funcs->p_vkCreateDescriptorSetLayout(device, pCreateInfo, pAllocator, pSetLayout);
}

VkResult WINAPI wine_vkCreateDescriptorUpdateTemplate(VkDevice device, const VkDescriptorUpdateTemplateCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDescriptorUpdateTemplate *pDescriptorUpdateTemplate)
{
    return unix_funcs->p_vkCreateDescriptorUpdateTemplate(device, pCreateInfo, pAllocator, pDescriptorUpdateTemplate);
}

VkResult WINAPI wine_vkCreateDescriptorUpdateTemplateKHR(VkDevice device, const VkDescriptorUpdateTemplateCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDescriptorUpdateTemplate *pDescriptorUpdateTemplate)
{
    return unix_funcs->p_vkCreateDescriptorUpdateTemplateKHR(device, pCreateInfo, pAllocator, pDescriptorUpdateTemplate);
}

VkResult WINAPI wine_vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDevice *pDevice)
{
    return unix_funcs->p_vkCreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);
}

VkResult WINAPI wine_vkCreateEvent(VkDevice device, const VkEventCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkEvent *pEvent)
{
    return unix_funcs->p_vkCreateEvent(device, pCreateInfo, pAllocator, pEvent);
}

VkResult WINAPI wine_vkCreateFence(VkDevice device, const VkFenceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkFence *pFence)
{
    return unix_funcs->p_vkCreateFence(device, pCreateInfo, pAllocator, pFence);
}

VkResult WINAPI wine_vkCreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkFramebuffer *pFramebuffer)
{
    return unix_funcs->p_vkCreateFramebuffer(device, pCreateInfo, pAllocator, pFramebuffer);
}

VkResult WINAPI wine_vkCreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines)
{
    return unix_funcs->p_vkCreateGraphicsPipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
}

VkResult WINAPI wine_vkCreateImage(VkDevice device, const VkImageCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkImage *pImage)
{
    return unix_funcs->p_vkCreateImage(device, pCreateInfo, pAllocator, pImage);
}

VkResult WINAPI wine_vkCreateImageView(VkDevice device, const VkImageViewCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkImageView *pView)
{
    return unix_funcs->p_vkCreateImageView(device, pCreateInfo, pAllocator, pView);
}

VkResult WINAPI wine_vkCreateIndirectCommandsLayoutNV(VkDevice device, const VkIndirectCommandsLayoutCreateInfoNV *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkIndirectCommandsLayoutNV *pIndirectCommandsLayout)
{
    return unix_funcs->p_vkCreateIndirectCommandsLayoutNV(device, pCreateInfo, pAllocator, pIndirectCommandsLayout);
}

VkResult WINAPI wine_vkCreatePipelineCache(VkDevice device, const VkPipelineCacheCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkPipelineCache *pPipelineCache)
{
    return unix_funcs->p_vkCreatePipelineCache(device, pCreateInfo, pAllocator, pPipelineCache);
}

VkResult WINAPI wine_vkCreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkPipelineLayout *pPipelineLayout)
{
    return unix_funcs->p_vkCreatePipelineLayout(device, pCreateInfo, pAllocator, pPipelineLayout);
}

VkResult WINAPI wine_vkCreatePrivateDataSlotEXT(VkDevice device, const VkPrivateDataSlotCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkPrivateDataSlotEXT *pPrivateDataSlot)
{
    return unix_funcs->p_vkCreatePrivateDataSlotEXT(device, pCreateInfo, pAllocator, pPrivateDataSlot);
}

VkResult WINAPI wine_vkCreateQueryPool(VkDevice device, const VkQueryPoolCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkQueryPool *pQueryPool)
{
    return unix_funcs->p_vkCreateQueryPool(device, pCreateInfo, pAllocator, pQueryPool);
}

VkResult WINAPI wine_vkCreateRayTracingPipelinesKHR(VkDevice device, VkDeferredOperationKHR deferredOperation, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkRayTracingPipelineCreateInfoKHR *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines)
{
    return unix_funcs->p_vkCreateRayTracingPipelinesKHR(device, deferredOperation, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
}

VkResult WINAPI wine_vkCreateRayTracingPipelinesNV(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkRayTracingPipelineCreateInfoNV *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines)
{
    return unix_funcs->p_vkCreateRayTracingPipelinesNV(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
}

VkResult WINAPI wine_vkCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkRenderPass *pRenderPass)
{
    return unix_funcs->p_vkCreateRenderPass(device, pCreateInfo, pAllocator, pRenderPass);
}

VkResult WINAPI wine_vkCreateRenderPass2(VkDevice device, const VkRenderPassCreateInfo2 *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkRenderPass *pRenderPass)
{
    return unix_funcs->p_vkCreateRenderPass2(device, pCreateInfo, pAllocator, pRenderPass);
}

VkResult WINAPI wine_vkCreateRenderPass2KHR(VkDevice device, const VkRenderPassCreateInfo2 *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkRenderPass *pRenderPass)
{
    return unix_funcs->p_vkCreateRenderPass2KHR(device, pCreateInfo, pAllocator, pRenderPass);
}

VkResult WINAPI wine_vkCreateSampler(VkDevice device, const VkSamplerCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSampler *pSampler)
{
    return unix_funcs->p_vkCreateSampler(device, pCreateInfo, pAllocator, pSampler);
}

VkResult WINAPI wine_vkCreateSamplerYcbcrConversion(VkDevice device, const VkSamplerYcbcrConversionCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSamplerYcbcrConversion *pYcbcrConversion)
{
    return unix_funcs->p_vkCreateSamplerYcbcrConversion(device, pCreateInfo, pAllocator, pYcbcrConversion);
}

VkResult WINAPI wine_vkCreateSamplerYcbcrConversionKHR(VkDevice device, const VkSamplerYcbcrConversionCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSamplerYcbcrConversion *pYcbcrConversion)
{
    return unix_funcs->p_vkCreateSamplerYcbcrConversionKHR(device, pCreateInfo, pAllocator, pYcbcrConversion);
}

VkResult WINAPI wine_vkCreateSemaphore(VkDevice device, const VkSemaphoreCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSemaphore *pSemaphore)
{
    return unix_funcs->p_vkCreateSemaphore(device, pCreateInfo, pAllocator, pSemaphore);
}

VkResult WINAPI wine_vkCreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkShaderModule *pShaderModule)
{
    return unix_funcs->p_vkCreateShaderModule(device, pCreateInfo, pAllocator, pShaderModule);
}

VkResult WINAPI wine_vkCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSwapchainKHR *pSwapchain)
{
    return unix_funcs->p_vkCreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);
}

VkResult WINAPI wine_vkCreateValidationCacheEXT(VkDevice device, const VkValidationCacheCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkValidationCacheEXT *pValidationCache)
{
    return unix_funcs->p_vkCreateValidationCacheEXT(device, pCreateInfo, pAllocator, pValidationCache);
}

VkResult WINAPI wine_vkCreateWin32SurfaceKHR(VkInstance instance, const VkWin32SurfaceCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface)
{
    return unix_funcs->p_vkCreateWin32SurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
}

VkResult WINAPI wine_vkDebugMarkerSetObjectNameEXT(VkDevice device, const VkDebugMarkerObjectNameInfoEXT *pNameInfo)
{
    return unix_funcs->p_vkDebugMarkerSetObjectNameEXT(device, pNameInfo);
}

VkResult WINAPI wine_vkDebugMarkerSetObjectTagEXT(VkDevice device, const VkDebugMarkerObjectTagInfoEXT *pTagInfo)
{
    return unix_funcs->p_vkDebugMarkerSetObjectTagEXT(device, pTagInfo);
}

void WINAPI wine_vkDebugReportMessageEXT(VkInstance instance, VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char *pLayerPrefix, const char *pMessage)
{
    unix_funcs->p_vkDebugReportMessageEXT(instance, flags, objectType, object, location, messageCode, pLayerPrefix, pMessage);
}

VkResult WINAPI wine_vkDeferredOperationJoinKHR(VkDevice device, VkDeferredOperationKHR operation)
{
    return unix_funcs->p_vkDeferredOperationJoinKHR(device, operation);
}

void WINAPI wine_vkDestroyAccelerationStructureKHR(VkDevice device, VkAccelerationStructureKHR accelerationStructure, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyAccelerationStructureKHR(device, accelerationStructure, pAllocator);
}

void WINAPI wine_vkDestroyAccelerationStructureNV(VkDevice device, VkAccelerationStructureNV accelerationStructure, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyAccelerationStructureNV(device, accelerationStructure, pAllocator);
}

void WINAPI wine_vkDestroyBuffer(VkDevice device, VkBuffer buffer, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyBuffer(device, buffer, pAllocator);
}

void WINAPI wine_vkDestroyBufferView(VkDevice device, VkBufferView bufferView, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyBufferView(device, bufferView, pAllocator);
}

void WINAPI wine_vkDestroyCommandPool(VkDevice device, VkCommandPool commandPool, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyCommandPool(device, commandPool, pAllocator);
}

void WINAPI wine_vkDestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyDebugReportCallbackEXT(instance, callback, pAllocator);
}

void WINAPI wine_vkDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyDebugUtilsMessengerEXT(instance, messenger, pAllocator);
}

void WINAPI wine_vkDestroyDeferredOperationKHR(VkDevice device, VkDeferredOperationKHR operation, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyDeferredOperationKHR(device, operation, pAllocator);
}

void WINAPI wine_vkDestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyDescriptorPool(device, descriptorPool, pAllocator);
}

void WINAPI wine_vkDestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyDescriptorSetLayout(device, descriptorSetLayout, pAllocator);
}

void WINAPI wine_vkDestroyDescriptorUpdateTemplate(VkDevice device, VkDescriptorUpdateTemplate descriptorUpdateTemplate, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyDescriptorUpdateTemplate(device, descriptorUpdateTemplate, pAllocator);
}

void WINAPI wine_vkDestroyDescriptorUpdateTemplateKHR(VkDevice device, VkDescriptorUpdateTemplate descriptorUpdateTemplate, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyDescriptorUpdateTemplateKHR(device, descriptorUpdateTemplate, pAllocator);
}

void WINAPI wine_vkDestroyDevice(VkDevice device, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyDevice(device, pAllocator);
}

void WINAPI wine_vkDestroyEvent(VkDevice device, VkEvent event, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyEvent(device, event, pAllocator);
}

void WINAPI wine_vkDestroyFence(VkDevice device, VkFence fence, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyFence(device, fence, pAllocator);
}

void WINAPI wine_vkDestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyFramebuffer(device, framebuffer, pAllocator);
}

void WINAPI wine_vkDestroyImage(VkDevice device, VkImage image, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyImage(device, image, pAllocator);
}

void WINAPI wine_vkDestroyImageView(VkDevice device, VkImageView imageView, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyImageView(device, imageView, pAllocator);
}

void WINAPI wine_vkDestroyIndirectCommandsLayoutNV(VkDevice device, VkIndirectCommandsLayoutNV indirectCommandsLayout, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyIndirectCommandsLayoutNV(device, indirectCommandsLayout, pAllocator);
}

void WINAPI wine_vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyInstance(instance, pAllocator);
}

void WINAPI wine_vkDestroyPipeline(VkDevice device, VkPipeline pipeline, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyPipeline(device, pipeline, pAllocator);
}

void WINAPI wine_vkDestroyPipelineCache(VkDevice device, VkPipelineCache pipelineCache, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyPipelineCache(device, pipelineCache, pAllocator);
}

void WINAPI wine_vkDestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyPipelineLayout(device, pipelineLayout, pAllocator);
}

void WINAPI wine_vkDestroyPrivateDataSlotEXT(VkDevice device, VkPrivateDataSlotEXT privateDataSlot, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyPrivateDataSlotEXT(device, privateDataSlot, pAllocator);
}

void WINAPI wine_vkDestroyQueryPool(VkDevice device, VkQueryPool queryPool, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyQueryPool(device, queryPool, pAllocator);
}

void WINAPI wine_vkDestroyRenderPass(VkDevice device, VkRenderPass renderPass, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyRenderPass(device, renderPass, pAllocator);
}

void WINAPI wine_vkDestroySampler(VkDevice device, VkSampler sampler, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroySampler(device, sampler, pAllocator);
}

void WINAPI wine_vkDestroySamplerYcbcrConversion(VkDevice device, VkSamplerYcbcrConversion ycbcrConversion, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroySamplerYcbcrConversion(device, ycbcrConversion, pAllocator);
}

void WINAPI wine_vkDestroySamplerYcbcrConversionKHR(VkDevice device, VkSamplerYcbcrConversion ycbcrConversion, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroySamplerYcbcrConversionKHR(device, ycbcrConversion, pAllocator);
}

void WINAPI wine_vkDestroySemaphore(VkDevice device, VkSemaphore semaphore, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroySemaphore(device, semaphore, pAllocator);
}

void WINAPI wine_vkDestroyShaderModule(VkDevice device, VkShaderModule shaderModule, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyShaderModule(device, shaderModule, pAllocator);
}

void WINAPI wine_vkDestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroySurfaceKHR(instance, surface, pAllocator);
}

void WINAPI wine_vkDestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroySwapchainKHR(device, swapchain, pAllocator);
}

void WINAPI wine_vkDestroyValidationCacheEXT(VkDevice device, VkValidationCacheEXT validationCache, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyValidationCacheEXT(device, validationCache, pAllocator);
}

VkResult WINAPI wine_vkDeviceWaitIdle(VkDevice device)
{
    return unix_funcs->p_vkDeviceWaitIdle(device);
}

VkResult WINAPI wine_vkEndCommandBuffer(VkCommandBuffer commandBuffer)
{
    return unix_funcs->p_vkEndCommandBuffer(commandBuffer);
}

VkResult WINAPI wine_vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char *pLayerName, uint32_t *pPropertyCount, VkExtensionProperties *pProperties)
{
    return unix_funcs->p_vkEnumerateDeviceExtensionProperties(physicalDevice, pLayerName, pPropertyCount, pProperties);
}

VkResult WINAPI wine_vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount, VkLayerProperties *pProperties)
{
    return unix_funcs->p_vkEnumerateDeviceLayerProperties(physicalDevice, pPropertyCount, pProperties);
}

VkResult WINAPI wine_vkEnumeratePhysicalDeviceGroups(VkInstance instance, uint32_t *pPhysicalDeviceGroupCount, VkPhysicalDeviceGroupProperties *pPhysicalDeviceGroupProperties)
{
    return unix_funcs->p_vkEnumeratePhysicalDeviceGroups(instance, pPhysicalDeviceGroupCount, pPhysicalDeviceGroupProperties);
}

VkResult WINAPI wine_vkEnumeratePhysicalDeviceGroupsKHR(VkInstance instance, uint32_t *pPhysicalDeviceGroupCount, VkPhysicalDeviceGroupProperties *pPhysicalDeviceGroupProperties)
{
    return unix_funcs->p_vkEnumeratePhysicalDeviceGroupsKHR(instance, pPhysicalDeviceGroupCount, pPhysicalDeviceGroupProperties);
}

VkResult WINAPI wine_vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, uint32_t *pCounterCount, VkPerformanceCounterKHR *pCounters, VkPerformanceCounterDescriptionKHR *pCounterDescriptions)
{
    return unix_funcs->p_vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR(physicalDevice, queueFamilyIndex, pCounterCount, pCounters, pCounterDescriptions);
}

VkResult WINAPI wine_vkEnumeratePhysicalDevices(VkInstance instance, uint32_t *pPhysicalDeviceCount, VkPhysicalDevice *pPhysicalDevices)
{
    return unix_funcs->p_vkEnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices);
}

VkResult WINAPI wine_vkFlushMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount, const VkMappedMemoryRange *pMemoryRanges)
{
    return unix_funcs->p_vkFlushMappedMemoryRanges(device, memoryRangeCount, pMemoryRanges);
}

void WINAPI wine_vkFreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount, const VkCommandBuffer *pCommandBuffers)
{
    unix_funcs->p_vkFreeCommandBuffers(device, commandPool, commandBufferCount, pCommandBuffers);
}

VkResult WINAPI wine_vkFreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t descriptorSetCount, const VkDescriptorSet *pDescriptorSets)
{
    return unix_funcs->p_vkFreeDescriptorSets(device, descriptorPool, descriptorSetCount, pDescriptorSets);
}

void WINAPI wine_vkFreeMemory(VkDevice device, VkDeviceMemory memory, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkFreeMemory(device, memory, pAllocator);
}

void WINAPI wine_vkGetAccelerationStructureBuildSizesKHR(VkDevice device, VkAccelerationStructureBuildTypeKHR buildType, const VkAccelerationStructureBuildGeometryInfoKHR *pBuildInfo, const uint32_t *pMaxPrimitiveCounts, VkAccelerationStructureBuildSizesInfoKHR *pSizeInfo)
{
    unix_funcs->p_vkGetAccelerationStructureBuildSizesKHR(device, buildType, pBuildInfo, pMaxPrimitiveCounts, pSizeInfo);
}

VkDeviceAddress WINAPI wine_vkGetAccelerationStructureDeviceAddressKHR(VkDevice device, const VkAccelerationStructureDeviceAddressInfoKHR *pInfo)
{
    return unix_funcs->p_vkGetAccelerationStructureDeviceAddressKHR(device, pInfo);
}

VkResult WINAPI wine_vkGetAccelerationStructureHandleNV(VkDevice device, VkAccelerationStructureNV accelerationStructure, size_t dataSize, void *pData)
{
    return unix_funcs->p_vkGetAccelerationStructureHandleNV(device, accelerationStructure, dataSize, pData);
}

void WINAPI wine_vkGetAccelerationStructureMemoryRequirementsNV(VkDevice device, const VkAccelerationStructureMemoryRequirementsInfoNV *pInfo, VkMemoryRequirements2KHR *pMemoryRequirements)
{
    unix_funcs->p_vkGetAccelerationStructureMemoryRequirementsNV(device, pInfo, pMemoryRequirements);
}

VkDeviceAddress WINAPI wine_vkGetBufferDeviceAddress(VkDevice device, const VkBufferDeviceAddressInfo *pInfo)
{
    return unix_funcs->p_vkGetBufferDeviceAddress(device, pInfo);
}

VkDeviceAddress WINAPI wine_vkGetBufferDeviceAddressEXT(VkDevice device, const VkBufferDeviceAddressInfo *pInfo)
{
    return unix_funcs->p_vkGetBufferDeviceAddressEXT(device, pInfo);
}

VkDeviceAddress WINAPI wine_vkGetBufferDeviceAddressKHR(VkDevice device, const VkBufferDeviceAddressInfo *pInfo)
{
    return unix_funcs->p_vkGetBufferDeviceAddressKHR(device, pInfo);
}

void WINAPI wine_vkGetBufferMemoryRequirements(VkDevice device, VkBuffer buffer, VkMemoryRequirements *pMemoryRequirements)
{
    unix_funcs->p_vkGetBufferMemoryRequirements(device, buffer, pMemoryRequirements);
}

void WINAPI wine_vkGetBufferMemoryRequirements2(VkDevice device, const VkBufferMemoryRequirementsInfo2 *pInfo, VkMemoryRequirements2 *pMemoryRequirements)
{
    unix_funcs->p_vkGetBufferMemoryRequirements2(device, pInfo, pMemoryRequirements);
}

void WINAPI wine_vkGetBufferMemoryRequirements2KHR(VkDevice device, const VkBufferMemoryRequirementsInfo2 *pInfo, VkMemoryRequirements2 *pMemoryRequirements)
{
    unix_funcs->p_vkGetBufferMemoryRequirements2KHR(device, pInfo, pMemoryRequirements);
}

uint64_t WINAPI wine_vkGetBufferOpaqueCaptureAddress(VkDevice device, const VkBufferDeviceAddressInfo *pInfo)
{
    return unix_funcs->p_vkGetBufferOpaqueCaptureAddress(device, pInfo);
}

uint64_t WINAPI wine_vkGetBufferOpaqueCaptureAddressKHR(VkDevice device, const VkBufferDeviceAddressInfo *pInfo)
{
    return unix_funcs->p_vkGetBufferOpaqueCaptureAddressKHR(device, pInfo);
}

VkResult WINAPI wine_vkGetCalibratedTimestampsEXT(VkDevice device, uint32_t timestampCount, const VkCalibratedTimestampInfoEXT *pTimestampInfos, uint64_t *pTimestamps, uint64_t *pMaxDeviation)
{
    return unix_funcs->p_vkGetCalibratedTimestampsEXT(device, timestampCount, pTimestampInfos, pTimestamps, pMaxDeviation);
}

uint32_t WINAPI wine_vkGetDeferredOperationMaxConcurrencyKHR(VkDevice device, VkDeferredOperationKHR operation)
{
    return unix_funcs->p_vkGetDeferredOperationMaxConcurrencyKHR(device, operation);
}

VkResult WINAPI wine_vkGetDeferredOperationResultKHR(VkDevice device, VkDeferredOperationKHR operation)
{
    return unix_funcs->p_vkGetDeferredOperationResultKHR(device, operation);
}

void WINAPI wine_vkGetDescriptorSetLayoutSupport(VkDevice device, const VkDescriptorSetLayoutCreateInfo *pCreateInfo, VkDescriptorSetLayoutSupport *pSupport)
{
    unix_funcs->p_vkGetDescriptorSetLayoutSupport(device, pCreateInfo, pSupport);
}

void WINAPI wine_vkGetDescriptorSetLayoutSupportKHR(VkDevice device, const VkDescriptorSetLayoutCreateInfo *pCreateInfo, VkDescriptorSetLayoutSupport *pSupport)
{
    unix_funcs->p_vkGetDescriptorSetLayoutSupportKHR(device, pCreateInfo, pSupport);
}

void WINAPI wine_vkGetDeviceAccelerationStructureCompatibilityKHR(VkDevice device, const VkAccelerationStructureVersionInfoKHR *pVersionInfo, VkAccelerationStructureCompatibilityKHR *pCompatibility)
{
    unix_funcs->p_vkGetDeviceAccelerationStructureCompatibilityKHR(device, pVersionInfo, pCompatibility);
}

void WINAPI wine_vkGetDeviceGroupPeerMemoryFeatures(VkDevice device, uint32_t heapIndex, uint32_t localDeviceIndex, uint32_t remoteDeviceIndex, VkPeerMemoryFeatureFlags *pPeerMemoryFeatures)
{
    unix_funcs->p_vkGetDeviceGroupPeerMemoryFeatures(device, heapIndex, localDeviceIndex, remoteDeviceIndex, pPeerMemoryFeatures);
}

void WINAPI wine_vkGetDeviceGroupPeerMemoryFeaturesKHR(VkDevice device, uint32_t heapIndex, uint32_t localDeviceIndex, uint32_t remoteDeviceIndex, VkPeerMemoryFeatureFlags *pPeerMemoryFeatures)
{
    unix_funcs->p_vkGetDeviceGroupPeerMemoryFeaturesKHR(device, heapIndex, localDeviceIndex, remoteDeviceIndex, pPeerMemoryFeatures);
}

VkResult WINAPI wine_vkGetDeviceGroupPresentCapabilitiesKHR(VkDevice device, VkDeviceGroupPresentCapabilitiesKHR *pDeviceGroupPresentCapabilities)
{
    return unix_funcs->p_vkGetDeviceGroupPresentCapabilitiesKHR(device, pDeviceGroupPresentCapabilities);
}

VkResult WINAPI wine_vkGetDeviceGroupSurfacePresentModesKHR(VkDevice device, VkSurfaceKHR surface, VkDeviceGroupPresentModeFlagsKHR *pModes)
{
    return unix_funcs->p_vkGetDeviceGroupSurfacePresentModesKHR(device, surface, pModes);
}

void WINAPI wine_vkGetDeviceMemoryCommitment(VkDevice device, VkDeviceMemory memory, VkDeviceSize *pCommittedMemoryInBytes)
{
    unix_funcs->p_vkGetDeviceMemoryCommitment(device, memory, pCommittedMemoryInBytes);
}

uint64_t WINAPI wine_vkGetDeviceMemoryOpaqueCaptureAddress(VkDevice device, const VkDeviceMemoryOpaqueCaptureAddressInfo *pInfo)
{
    return unix_funcs->p_vkGetDeviceMemoryOpaqueCaptureAddress(device, pInfo);
}

uint64_t WINAPI wine_vkGetDeviceMemoryOpaqueCaptureAddressKHR(VkDevice device, const VkDeviceMemoryOpaqueCaptureAddressInfo *pInfo)
{
    return unix_funcs->p_vkGetDeviceMemoryOpaqueCaptureAddressKHR(device, pInfo);
}

void WINAPI wine_vkGetDeviceQueue(VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue *pQueue)
{
    unix_funcs->p_vkGetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);
}

void WINAPI wine_vkGetDeviceQueue2(VkDevice device, const VkDeviceQueueInfo2 *pQueueInfo, VkQueue *pQueue)
{
    unix_funcs->p_vkGetDeviceQueue2(device, pQueueInfo, pQueue);
}

VkResult WINAPI wine_vkGetEventStatus(VkDevice device, VkEvent event)
{
    return unix_funcs->p_vkGetEventStatus(device, event);
}

VkResult WINAPI wine_vkGetFenceStatus(VkDevice device, VkFence fence)
{
    return unix_funcs->p_vkGetFenceStatus(device, fence);
}

void WINAPI wine_vkGetGeneratedCommandsMemoryRequirementsNV(VkDevice device, const VkGeneratedCommandsMemoryRequirementsInfoNV *pInfo, VkMemoryRequirements2 *pMemoryRequirements)
{
    unix_funcs->p_vkGetGeneratedCommandsMemoryRequirementsNV(device, pInfo, pMemoryRequirements);
}

void WINAPI wine_vkGetImageMemoryRequirements(VkDevice device, VkImage image, VkMemoryRequirements *pMemoryRequirements)
{
    unix_funcs->p_vkGetImageMemoryRequirements(device, image, pMemoryRequirements);
}

void WINAPI wine_vkGetImageMemoryRequirements2(VkDevice device, const VkImageMemoryRequirementsInfo2 *pInfo, VkMemoryRequirements2 *pMemoryRequirements)
{
    unix_funcs->p_vkGetImageMemoryRequirements2(device, pInfo, pMemoryRequirements);
}

void WINAPI wine_vkGetImageMemoryRequirements2KHR(VkDevice device, const VkImageMemoryRequirementsInfo2 *pInfo, VkMemoryRequirements2 *pMemoryRequirements)
{
    unix_funcs->p_vkGetImageMemoryRequirements2KHR(device, pInfo, pMemoryRequirements);
}

void WINAPI wine_vkGetImageSparseMemoryRequirements(VkDevice device, VkImage image, uint32_t *pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements *pSparseMemoryRequirements)
{
    unix_funcs->p_vkGetImageSparseMemoryRequirements(device, image, pSparseMemoryRequirementCount, pSparseMemoryRequirements);
}

void WINAPI wine_vkGetImageSparseMemoryRequirements2(VkDevice device, const VkImageSparseMemoryRequirementsInfo2 *pInfo, uint32_t *pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements2 *pSparseMemoryRequirements)
{
    unix_funcs->p_vkGetImageSparseMemoryRequirements2(device, pInfo, pSparseMemoryRequirementCount, pSparseMemoryRequirements);
}

void WINAPI wine_vkGetImageSparseMemoryRequirements2KHR(VkDevice device, const VkImageSparseMemoryRequirementsInfo2 *pInfo, uint32_t *pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements2 *pSparseMemoryRequirements)
{
    unix_funcs->p_vkGetImageSparseMemoryRequirements2KHR(device, pInfo, pSparseMemoryRequirementCount, pSparseMemoryRequirements);
}

void WINAPI wine_vkGetImageSubresourceLayout(VkDevice device, VkImage image, const VkImageSubresource *pSubresource, VkSubresourceLayout *pLayout)
{
    unix_funcs->p_vkGetImageSubresourceLayout(device, image, pSubresource, pLayout);
}

VkResult WINAPI wine_vkGetMemoryHostPointerPropertiesEXT(VkDevice device, VkExternalMemoryHandleTypeFlagBits handleType, const void *pHostPointer, VkMemoryHostPointerPropertiesEXT *pMemoryHostPointerProperties)
{
    return unix_funcs->p_vkGetMemoryHostPointerPropertiesEXT(device, handleType, pHostPointer, pMemoryHostPointerProperties);
}

VkResult WINAPI wine_vkGetPerformanceParameterINTEL(VkDevice device, VkPerformanceParameterTypeINTEL parameter, VkPerformanceValueINTEL *pValue)
{
    return unix_funcs->p_vkGetPerformanceParameterINTEL(device, parameter, pValue);
}

VkResult WINAPI wine_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT(VkPhysicalDevice physicalDevice, uint32_t *pTimeDomainCount, VkTimeDomainEXT *pTimeDomains)
{
    return unix_funcs->p_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT(physicalDevice, pTimeDomainCount, pTimeDomains);
}

VkResult WINAPI wine_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV(VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount, VkCooperativeMatrixPropertiesNV *pProperties)
{
    return unix_funcs->p_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV(physicalDevice, pPropertyCount, pProperties);
}

void WINAPI wine_vkGetPhysicalDeviceExternalBufferProperties(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalBufferInfo *pExternalBufferInfo, VkExternalBufferProperties *pExternalBufferProperties)
{
    unix_funcs->p_vkGetPhysicalDeviceExternalBufferProperties(physicalDevice, pExternalBufferInfo, pExternalBufferProperties);
}

void WINAPI wine_vkGetPhysicalDeviceExternalBufferPropertiesKHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalBufferInfo *pExternalBufferInfo, VkExternalBufferProperties *pExternalBufferProperties)
{
    unix_funcs->p_vkGetPhysicalDeviceExternalBufferPropertiesKHR(physicalDevice, pExternalBufferInfo, pExternalBufferProperties);
}

void WINAPI wine_vkGetPhysicalDeviceExternalFenceProperties(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalFenceInfo *pExternalFenceInfo, VkExternalFenceProperties *pExternalFenceProperties)
{
    unix_funcs->p_vkGetPhysicalDeviceExternalFenceProperties(physicalDevice, pExternalFenceInfo, pExternalFenceProperties);
}

void WINAPI wine_vkGetPhysicalDeviceExternalFencePropertiesKHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalFenceInfo *pExternalFenceInfo, VkExternalFenceProperties *pExternalFenceProperties)
{
    unix_funcs->p_vkGetPhysicalDeviceExternalFencePropertiesKHR(physicalDevice, pExternalFenceInfo, pExternalFenceProperties);
}

void WINAPI wine_vkGetPhysicalDeviceExternalSemaphoreProperties(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalSemaphoreInfo *pExternalSemaphoreInfo, VkExternalSemaphoreProperties *pExternalSemaphoreProperties)
{
    unix_funcs->p_vkGetPhysicalDeviceExternalSemaphoreProperties(physicalDevice, pExternalSemaphoreInfo, pExternalSemaphoreProperties);
}

void WINAPI wine_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalSemaphoreInfo *pExternalSemaphoreInfo, VkExternalSemaphoreProperties *pExternalSemaphoreProperties)
{
    unix_funcs->p_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR(physicalDevice, pExternalSemaphoreInfo, pExternalSemaphoreProperties);
}

void WINAPI wine_vkGetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures *pFeatures)
{
    unix_funcs->p_vkGetPhysicalDeviceFeatures(physicalDevice, pFeatures);
}

void WINAPI wine_vkGetPhysicalDeviceFeatures2(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2 *pFeatures)
{
    unix_funcs->p_vkGetPhysicalDeviceFeatures2(physicalDevice, pFeatures);
}

void WINAPI wine_vkGetPhysicalDeviceFeatures2KHR(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2 *pFeatures)
{
    unix_funcs->p_vkGetPhysicalDeviceFeatures2KHR(physicalDevice, pFeatures);
}

void WINAPI wine_vkGetPhysicalDeviceFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties *pFormatProperties)
{
    unix_funcs->p_vkGetPhysicalDeviceFormatProperties(physicalDevice, format, pFormatProperties);
}

void WINAPI wine_vkGetPhysicalDeviceFormatProperties2(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties2 *pFormatProperties)
{
    unix_funcs->p_vkGetPhysicalDeviceFormatProperties2(physicalDevice, format, pFormatProperties);
}

void WINAPI wine_vkGetPhysicalDeviceFormatProperties2KHR(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties2 *pFormatProperties)
{
    unix_funcs->p_vkGetPhysicalDeviceFormatProperties2KHR(physicalDevice, format, pFormatProperties);
}

VkResult WINAPI wine_vkGetPhysicalDeviceFragmentShadingRatesKHR(VkPhysicalDevice physicalDevice, uint32_t *pFragmentShadingRateCount, VkPhysicalDeviceFragmentShadingRateKHR *pFragmentShadingRates)
{
    return unix_funcs->p_vkGetPhysicalDeviceFragmentShadingRatesKHR(physicalDevice, pFragmentShadingRateCount, pFragmentShadingRates);
}

VkResult WINAPI wine_vkGetPhysicalDeviceImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, VkImageFormatProperties *pImageFormatProperties)
{
    return unix_funcs->p_vkGetPhysicalDeviceImageFormatProperties(physicalDevice, format, type, tiling, usage, flags, pImageFormatProperties);
}

VkResult WINAPI wine_vkGetPhysicalDeviceImageFormatProperties2(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceImageFormatInfo2 *pImageFormatInfo, VkImageFormatProperties2 *pImageFormatProperties)
{
    return unix_funcs->p_vkGetPhysicalDeviceImageFormatProperties2(physicalDevice, pImageFormatInfo, pImageFormatProperties);
}

VkResult WINAPI wine_vkGetPhysicalDeviceImageFormatProperties2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceImageFormatInfo2 *pImageFormatInfo, VkImageFormatProperties2 *pImageFormatProperties)
{
    return unix_funcs->p_vkGetPhysicalDeviceImageFormatProperties2KHR(physicalDevice, pImageFormatInfo, pImageFormatProperties);
}

void WINAPI wine_vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties *pMemoryProperties)
{
    unix_funcs->p_vkGetPhysicalDeviceMemoryProperties(physicalDevice, pMemoryProperties);
}

void WINAPI wine_vkGetPhysicalDeviceMemoryProperties2(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties2 *pMemoryProperties)
{
    unix_funcs->p_vkGetPhysicalDeviceMemoryProperties2(physicalDevice, pMemoryProperties);
}

void WINAPI wine_vkGetPhysicalDeviceMemoryProperties2KHR(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties2 *pMemoryProperties)
{
    unix_funcs->p_vkGetPhysicalDeviceMemoryProperties2KHR(physicalDevice, pMemoryProperties);
}

void WINAPI wine_vkGetPhysicalDeviceMultisamplePropertiesEXT(VkPhysicalDevice physicalDevice, VkSampleCountFlagBits samples, VkMultisamplePropertiesEXT *pMultisampleProperties)
{
    unix_funcs->p_vkGetPhysicalDeviceMultisamplePropertiesEXT(physicalDevice, samples, pMultisampleProperties);
}

VkResult WINAPI wine_vkGetPhysicalDevicePresentRectanglesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t *pRectCount, VkRect2D *pRects)
{
    return unix_funcs->p_vkGetPhysicalDevicePresentRectanglesKHR(physicalDevice, surface, pRectCount, pRects);
}

void WINAPI wine_vkGetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties *pProperties)
{
    unix_funcs->p_vkGetPhysicalDeviceProperties(physicalDevice, pProperties);
}

void WINAPI wine_vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR(VkPhysicalDevice physicalDevice, const VkQueryPoolPerformanceCreateInfoKHR *pPerformanceQueryCreateInfo, uint32_t *pNumPasses)
{
    unix_funcs->p_vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR(physicalDevice, pPerformanceQueryCreateInfo, pNumPasses);
}

void WINAPI wine_vkGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice, uint32_t *pQueueFamilyPropertyCount, VkQueueFamilyProperties *pQueueFamilyProperties)
{
    unix_funcs->p_vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
}

void WINAPI wine_vkGetPhysicalDeviceQueueFamilyProperties2(VkPhysicalDevice physicalDevice, uint32_t *pQueueFamilyPropertyCount, VkQueueFamilyProperties2 *pQueueFamilyProperties)
{
    unix_funcs->p_vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
}

void WINAPI wine_vkGetPhysicalDeviceQueueFamilyProperties2KHR(VkPhysicalDevice physicalDevice, uint32_t *pQueueFamilyPropertyCount, VkQueueFamilyProperties2 *pQueueFamilyProperties)
{
    unix_funcs->p_vkGetPhysicalDeviceQueueFamilyProperties2KHR(physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
}

void WINAPI wine_vkGetPhysicalDeviceSparseImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkSampleCountFlagBits samples, VkImageUsageFlags usage, VkImageTiling tiling, uint32_t *pPropertyCount, VkSparseImageFormatProperties *pProperties)
{
    unix_funcs->p_vkGetPhysicalDeviceSparseImageFormatProperties(physicalDevice, format, type, samples, usage, tiling, pPropertyCount, pProperties);
}

void WINAPI wine_vkGetPhysicalDeviceSparseImageFormatProperties2(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSparseImageFormatInfo2 *pFormatInfo, uint32_t *pPropertyCount, VkSparseImageFormatProperties2 *pProperties)
{
    unix_funcs->p_vkGetPhysicalDeviceSparseImageFormatProperties2(physicalDevice, pFormatInfo, pPropertyCount, pProperties);
}

void WINAPI wine_vkGetPhysicalDeviceSparseImageFormatProperties2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSparseImageFormatInfo2 *pFormatInfo, uint32_t *pPropertyCount, VkSparseImageFormatProperties2 *pProperties)
{
    unix_funcs->p_vkGetPhysicalDeviceSparseImageFormatProperties2KHR(physicalDevice, pFormatInfo, pPropertyCount, pProperties);
}

VkResult WINAPI wine_vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV(VkPhysicalDevice physicalDevice, uint32_t *pCombinationCount, VkFramebufferMixedSamplesCombinationNV *pCombinations)
{
    return unix_funcs->p_vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV(physicalDevice, pCombinationCount, pCombinations);
}

VkResult WINAPI wine_vkGetPhysicalDeviceSurfaceCapabilities2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR *pSurfaceInfo, VkSurfaceCapabilities2KHR *pSurfaceCapabilities)
{
    return unix_funcs->p_vkGetPhysicalDeviceSurfaceCapabilities2KHR(physicalDevice, pSurfaceInfo, pSurfaceCapabilities);
}

VkResult WINAPI wine_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR *pSurfaceCapabilities)
{
    return unix_funcs->p_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, pSurfaceCapabilities);
}

VkResult WINAPI wine_vkGetPhysicalDeviceSurfaceFormats2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR *pSurfaceInfo, uint32_t *pSurfaceFormatCount, VkSurfaceFormat2KHR *pSurfaceFormats)
{
    return unix_funcs->p_vkGetPhysicalDeviceSurfaceFormats2KHR(physicalDevice, pSurfaceInfo, pSurfaceFormatCount, pSurfaceFormats);
}

VkResult WINAPI wine_vkGetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t *pSurfaceFormatCount, VkSurfaceFormatKHR *pSurfaceFormats)
{
    return unix_funcs->p_vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, pSurfaceFormatCount, pSurfaceFormats);
}

VkResult WINAPI wine_vkGetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t *pPresentModeCount, VkPresentModeKHR *pPresentModes)
{
    return unix_funcs->p_vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, pPresentModeCount, pPresentModes);
}

VkResult WINAPI wine_vkGetPhysicalDeviceSurfaceSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, VkSurfaceKHR surface, VkBool32 *pSupported)
{
    return unix_funcs->p_vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, surface, pSupported);
}

VkResult WINAPI wine_vkGetPhysicalDeviceToolPropertiesEXT(VkPhysicalDevice physicalDevice, uint32_t *pToolCount, VkPhysicalDeviceToolPropertiesEXT *pToolProperties)
{
    return unix_funcs->p_vkGetPhysicalDeviceToolPropertiesEXT(physicalDevice, pToolCount, pToolProperties);
}

VkBool32 WINAPI wine_vkGetPhysicalDeviceWin32PresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex)
{
    return unix_funcs->p_vkGetPhysicalDeviceWin32PresentationSupportKHR(physicalDevice, queueFamilyIndex);
}

VkResult WINAPI wine_vkGetPipelineCacheData(VkDevice device, VkPipelineCache pipelineCache, size_t *pDataSize, void *pData)
{
    return unix_funcs->p_vkGetPipelineCacheData(device, pipelineCache, pDataSize, pData);
}

VkResult WINAPI wine_vkGetPipelineExecutableInternalRepresentationsKHR(VkDevice device, const VkPipelineExecutableInfoKHR *pExecutableInfo, uint32_t *pInternalRepresentationCount, VkPipelineExecutableInternalRepresentationKHR *pInternalRepresentations)
{
    return unix_funcs->p_vkGetPipelineExecutableInternalRepresentationsKHR(device, pExecutableInfo, pInternalRepresentationCount, pInternalRepresentations);
}

VkResult WINAPI wine_vkGetPipelineExecutablePropertiesKHR(VkDevice device, const VkPipelineInfoKHR *pPipelineInfo, uint32_t *pExecutableCount, VkPipelineExecutablePropertiesKHR *pProperties)
{
    return unix_funcs->p_vkGetPipelineExecutablePropertiesKHR(device, pPipelineInfo, pExecutableCount, pProperties);
}

VkResult WINAPI wine_vkGetPipelineExecutableStatisticsKHR(VkDevice device, const VkPipelineExecutableInfoKHR *pExecutableInfo, uint32_t *pStatisticCount, VkPipelineExecutableStatisticKHR *pStatistics)
{
    return unix_funcs->p_vkGetPipelineExecutableStatisticsKHR(device, pExecutableInfo, pStatisticCount, pStatistics);
}

void WINAPI wine_vkGetPrivateDataEXT(VkDevice device, VkObjectType objectType, uint64_t objectHandle, VkPrivateDataSlotEXT privateDataSlot, uint64_t *pData)
{
    unix_funcs->p_vkGetPrivateDataEXT(device, objectType, objectHandle, privateDataSlot, pData);
}

VkResult WINAPI wine_vkGetQueryPoolResults(VkDevice device, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, size_t dataSize, void *pData, VkDeviceSize stride, VkQueryResultFlags flags)
{
    return unix_funcs->p_vkGetQueryPoolResults(device, queryPool, firstQuery, queryCount, dataSize, pData, stride, flags);
}

void WINAPI wine_vkGetQueueCheckpointData2NV(VkQueue queue, uint32_t *pCheckpointDataCount, VkCheckpointData2NV *pCheckpointData)
{
    unix_funcs->p_vkGetQueueCheckpointData2NV(queue, pCheckpointDataCount, pCheckpointData);
}

void WINAPI wine_vkGetQueueCheckpointDataNV(VkQueue queue, uint32_t *pCheckpointDataCount, VkCheckpointDataNV *pCheckpointData)
{
    unix_funcs->p_vkGetQueueCheckpointDataNV(queue, pCheckpointDataCount, pCheckpointData);
}

VkResult WINAPI wine_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR(VkDevice device, VkPipeline pipeline, uint32_t firstGroup, uint32_t groupCount, size_t dataSize, void *pData)
{
    return unix_funcs->p_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR(device, pipeline, firstGroup, groupCount, dataSize, pData);
}

VkResult WINAPI wine_vkGetRayTracingShaderGroupHandlesKHR(VkDevice device, VkPipeline pipeline, uint32_t firstGroup, uint32_t groupCount, size_t dataSize, void *pData)
{
    return unix_funcs->p_vkGetRayTracingShaderGroupHandlesKHR(device, pipeline, firstGroup, groupCount, dataSize, pData);
}

VkResult WINAPI wine_vkGetRayTracingShaderGroupHandlesNV(VkDevice device, VkPipeline pipeline, uint32_t firstGroup, uint32_t groupCount, size_t dataSize, void *pData)
{
    return unix_funcs->p_vkGetRayTracingShaderGroupHandlesNV(device, pipeline, firstGroup, groupCount, dataSize, pData);
}

VkDeviceSize WINAPI wine_vkGetRayTracingShaderGroupStackSizeKHR(VkDevice device, VkPipeline pipeline, uint32_t group, VkShaderGroupShaderKHR groupShader)
{
    return unix_funcs->p_vkGetRayTracingShaderGroupStackSizeKHR(device, pipeline, group, groupShader);
}

void WINAPI wine_vkGetRenderAreaGranularity(VkDevice device, VkRenderPass renderPass, VkExtent2D *pGranularity)
{
    unix_funcs->p_vkGetRenderAreaGranularity(device, renderPass, pGranularity);
}

VkResult WINAPI wine_vkGetSemaphoreCounterValue(VkDevice device, VkSemaphore semaphore, uint64_t *pValue)
{
    return unix_funcs->p_vkGetSemaphoreCounterValue(device, semaphore, pValue);
}

VkResult WINAPI wine_vkGetSemaphoreCounterValueKHR(VkDevice device, VkSemaphore semaphore, uint64_t *pValue)
{
    return unix_funcs->p_vkGetSemaphoreCounterValueKHR(device, semaphore, pValue);
}

VkResult WINAPI wine_vkGetShaderInfoAMD(VkDevice device, VkPipeline pipeline, VkShaderStageFlagBits shaderStage, VkShaderInfoTypeAMD infoType, size_t *pInfoSize, void *pInfo)
{
    return unix_funcs->p_vkGetShaderInfoAMD(device, pipeline, shaderStage, infoType, pInfoSize, pInfo);
}

VkResult WINAPI wine_vkGetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t *pSwapchainImageCount, VkImage *pSwapchainImages)
{
    return unix_funcs->p_vkGetSwapchainImagesKHR(device, swapchain, pSwapchainImageCount, pSwapchainImages);
}

VkResult WINAPI wine_vkGetValidationCacheDataEXT(VkDevice device, VkValidationCacheEXT validationCache, size_t *pDataSize, void *pData)
{
    return unix_funcs->p_vkGetValidationCacheDataEXT(device, validationCache, pDataSize, pData);
}

VkResult WINAPI wine_vkInitializePerformanceApiINTEL(VkDevice device, const VkInitializePerformanceApiInfoINTEL *pInitializeInfo)
{
    return unix_funcs->p_vkInitializePerformanceApiINTEL(device, pInitializeInfo);
}

VkResult WINAPI wine_vkInvalidateMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount, const VkMappedMemoryRange *pMemoryRanges)
{
    return unix_funcs->p_vkInvalidateMappedMemoryRanges(device, memoryRangeCount, pMemoryRanges);
}

VkResult WINAPI wine_vkMapMemory(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void **ppData)
{
    return unix_funcs->p_vkMapMemory(device, memory, offset, size, flags, ppData);
}

VkResult WINAPI wine_vkMergePipelineCaches(VkDevice device, VkPipelineCache dstCache, uint32_t srcCacheCount, const VkPipelineCache *pSrcCaches)
{
    return unix_funcs->p_vkMergePipelineCaches(device, dstCache, srcCacheCount, pSrcCaches);
}

VkResult WINAPI wine_vkMergeValidationCachesEXT(VkDevice device, VkValidationCacheEXT dstCache, uint32_t srcCacheCount, const VkValidationCacheEXT *pSrcCaches)
{
    return unix_funcs->p_vkMergeValidationCachesEXT(device, dstCache, srcCacheCount, pSrcCaches);
}

void WINAPI wine_vkQueueBeginDebugUtilsLabelEXT(VkQueue queue, const VkDebugUtilsLabelEXT *pLabelInfo)
{
    unix_funcs->p_vkQueueBeginDebugUtilsLabelEXT(queue, pLabelInfo);
}

VkResult WINAPI wine_vkQueueBindSparse(VkQueue queue, uint32_t bindInfoCount, const VkBindSparseInfo *pBindInfo, VkFence fence)
{
    return unix_funcs->p_vkQueueBindSparse(queue, bindInfoCount, pBindInfo, fence);
}

void WINAPI wine_vkQueueEndDebugUtilsLabelEXT(VkQueue queue)
{
    unix_funcs->p_vkQueueEndDebugUtilsLabelEXT(queue);
}

void WINAPI wine_vkQueueInsertDebugUtilsLabelEXT(VkQueue queue, const VkDebugUtilsLabelEXT *pLabelInfo)
{
    unix_funcs->p_vkQueueInsertDebugUtilsLabelEXT(queue, pLabelInfo);
}

VkResult WINAPI wine_vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR *pPresentInfo)
{
    return unix_funcs->p_vkQueuePresentKHR(queue, pPresentInfo);
}

VkResult WINAPI wine_vkQueueSetPerformanceConfigurationINTEL(VkQueue queue, VkPerformanceConfigurationINTEL configuration)
{
    return unix_funcs->p_vkQueueSetPerformanceConfigurationINTEL(queue, configuration);
}

VkResult WINAPI wine_vkQueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo *pSubmits, VkFence fence)
{
    return unix_funcs->p_vkQueueSubmit(queue, submitCount, pSubmits, fence);
}

VkResult WINAPI wine_vkQueueSubmit2KHR(VkQueue queue, uint32_t submitCount, const VkSubmitInfo2KHR *pSubmits, VkFence fence)
{
    return unix_funcs->p_vkQueueSubmit2KHR(queue, submitCount, pSubmits, fence);
}

VkResult WINAPI wine_vkQueueWaitIdle(VkQueue queue)
{
    return unix_funcs->p_vkQueueWaitIdle(queue);
}

VkResult WINAPI wine_vkReleasePerformanceConfigurationINTEL(VkDevice device, VkPerformanceConfigurationINTEL configuration)
{
    return unix_funcs->p_vkReleasePerformanceConfigurationINTEL(device, configuration);
}

void WINAPI wine_vkReleaseProfilingLockKHR(VkDevice device)
{
    unix_funcs->p_vkReleaseProfilingLockKHR(device);
}

VkResult WINAPI wine_vkResetCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags)
{
    return unix_funcs->p_vkResetCommandBuffer(commandBuffer, flags);
}

VkResult WINAPI wine_vkResetCommandPool(VkDevice device, VkCommandPool commandPool, VkCommandPoolResetFlags flags)
{
    return unix_funcs->p_vkResetCommandPool(device, commandPool, flags);
}

VkResult WINAPI wine_vkResetDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorPoolResetFlags flags)
{
    return unix_funcs->p_vkResetDescriptorPool(device, descriptorPool, flags);
}

VkResult WINAPI wine_vkResetEvent(VkDevice device, VkEvent event)
{
    return unix_funcs->p_vkResetEvent(device, event);
}

VkResult WINAPI wine_vkResetFences(VkDevice device, uint32_t fenceCount, const VkFence *pFences)
{
    return unix_funcs->p_vkResetFences(device, fenceCount, pFences);
}

void WINAPI wine_vkResetQueryPool(VkDevice device, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount)
{
    unix_funcs->p_vkResetQueryPool(device, queryPool, firstQuery, queryCount);
}

void WINAPI wine_vkResetQueryPoolEXT(VkDevice device, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount)
{
    unix_funcs->p_vkResetQueryPoolEXT(device, queryPool, firstQuery, queryCount);
}

VkResult WINAPI wine_vkSetDebugUtilsObjectNameEXT(VkDevice device, const VkDebugUtilsObjectNameInfoEXT *pNameInfo)
{
    return unix_funcs->p_vkSetDebugUtilsObjectNameEXT(device, pNameInfo);
}

VkResult WINAPI wine_vkSetDebugUtilsObjectTagEXT(VkDevice device, const VkDebugUtilsObjectTagInfoEXT *pTagInfo)
{
    return unix_funcs->p_vkSetDebugUtilsObjectTagEXT(device, pTagInfo);
}

VkResult WINAPI wine_vkSetEvent(VkDevice device, VkEvent event)
{
    return unix_funcs->p_vkSetEvent(device, event);
}

VkResult WINAPI wine_vkSetPrivateDataEXT(VkDevice device, VkObjectType objectType, uint64_t objectHandle, VkPrivateDataSlotEXT privateDataSlot, uint64_t data)
{
    return unix_funcs->p_vkSetPrivateDataEXT(device, objectType, objectHandle, privateDataSlot, data);
}

VkResult WINAPI wine_vkSignalSemaphore(VkDevice device, const VkSemaphoreSignalInfo *pSignalInfo)
{
    return unix_funcs->p_vkSignalSemaphore(device, pSignalInfo);
}

VkResult WINAPI wine_vkSignalSemaphoreKHR(VkDevice device, const VkSemaphoreSignalInfo *pSignalInfo)
{
    return unix_funcs->p_vkSignalSemaphoreKHR(device, pSignalInfo);
}

void WINAPI wine_vkSubmitDebugUtilsMessageEXT(VkInstance instance, VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData)
{
    unix_funcs->p_vkSubmitDebugUtilsMessageEXT(instance, messageSeverity, messageTypes, pCallbackData);
}

void WINAPI wine_vkTrimCommandPool(VkDevice device, VkCommandPool commandPool, VkCommandPoolTrimFlags flags)
{
    unix_funcs->p_vkTrimCommandPool(device, commandPool, flags);
}

void WINAPI wine_vkTrimCommandPoolKHR(VkDevice device, VkCommandPool commandPool, VkCommandPoolTrimFlags flags)
{
    unix_funcs->p_vkTrimCommandPoolKHR(device, commandPool, flags);
}

void WINAPI wine_vkUninitializePerformanceApiINTEL(VkDevice device)
{
    unix_funcs->p_vkUninitializePerformanceApiINTEL(device);
}

void WINAPI wine_vkUnmapMemory(VkDevice device, VkDeviceMemory memory)
{
    unix_funcs->p_vkUnmapMemory(device, memory);
}

void WINAPI wine_vkUpdateDescriptorSetWithTemplate(VkDevice device, VkDescriptorSet descriptorSet, VkDescriptorUpdateTemplate descriptorUpdateTemplate, const void *pData)
{
    unix_funcs->p_vkUpdateDescriptorSetWithTemplate(device, descriptorSet, descriptorUpdateTemplate, pData);
}

void WINAPI wine_vkUpdateDescriptorSetWithTemplateKHR(VkDevice device, VkDescriptorSet descriptorSet, VkDescriptorUpdateTemplate descriptorUpdateTemplate, const void *pData)
{
    unix_funcs->p_vkUpdateDescriptorSetWithTemplateKHR(device, descriptorSet, descriptorUpdateTemplate, pData);
}

void WINAPI wine_vkUpdateDescriptorSets(VkDevice device, uint32_t descriptorWriteCount, const VkWriteDescriptorSet *pDescriptorWrites, uint32_t descriptorCopyCount, const VkCopyDescriptorSet *pDescriptorCopies)
{
    unix_funcs->p_vkUpdateDescriptorSets(device, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies);
}

VkResult WINAPI wine_vkWaitForFences(VkDevice device, uint32_t fenceCount, const VkFence *pFences, VkBool32 waitAll, uint64_t timeout)
{
    return unix_funcs->p_vkWaitForFences(device, fenceCount, pFences, waitAll, timeout);
}

VkResult WINAPI wine_vkWaitSemaphores(VkDevice device, const VkSemaphoreWaitInfo *pWaitInfo, uint64_t timeout)
{
    return unix_funcs->p_vkWaitSemaphores(device, pWaitInfo, timeout);
}

VkResult WINAPI wine_vkWaitSemaphoresKHR(VkDevice device, const VkSemaphoreWaitInfo *pWaitInfo, uint64_t timeout)
{
    return unix_funcs->p_vkWaitSemaphoresKHR(device, pWaitInfo, timeout);
}

VkResult WINAPI wine_vkWriteAccelerationStructuresPropertiesKHR(VkDevice device, uint32_t accelerationStructureCount, const VkAccelerationStructureKHR *pAccelerationStructures, VkQueryType queryType, size_t dataSize, void *pData, size_t stride)
{
    return unix_funcs->p_vkWriteAccelerationStructuresPropertiesKHR(device, accelerationStructureCount, pAccelerationStructures, queryType, dataSize, pData, stride);
}

static const struct vulkan_func vk_device_dispatch_table[] =
{
    {"vkAcquireNextImage2KHR", &wine_vkAcquireNextImage2KHR},
    {"vkAcquireNextImageKHR", &wine_vkAcquireNextImageKHR},
    {"vkAcquirePerformanceConfigurationINTEL", &wine_vkAcquirePerformanceConfigurationINTEL},
    {"vkAcquireProfilingLockKHR", &wine_vkAcquireProfilingLockKHR},
    {"vkAllocateCommandBuffers", &wine_vkAllocateCommandBuffers},
    {"vkAllocateDescriptorSets", &wine_vkAllocateDescriptorSets},
    {"vkAllocateMemory", &wine_vkAllocateMemory},
    {"vkBeginCommandBuffer", &wine_vkBeginCommandBuffer},
    {"vkBindAccelerationStructureMemoryNV", &wine_vkBindAccelerationStructureMemoryNV},
    {"vkBindBufferMemory", &wine_vkBindBufferMemory},
    {"vkBindBufferMemory2", &wine_vkBindBufferMemory2},
    {"vkBindBufferMemory2KHR", &wine_vkBindBufferMemory2KHR},
    {"vkBindImageMemory", &wine_vkBindImageMemory},
    {"vkBindImageMemory2", &wine_vkBindImageMemory2},
    {"vkBindImageMemory2KHR", &wine_vkBindImageMemory2KHR},
    {"vkBuildAccelerationStructuresKHR", &wine_vkBuildAccelerationStructuresKHR},
    {"vkCmdBeginConditionalRenderingEXT", &wine_vkCmdBeginConditionalRenderingEXT},
    {"vkCmdBeginDebugUtilsLabelEXT", &wine_vkCmdBeginDebugUtilsLabelEXT},
    {"vkCmdBeginQuery", &wine_vkCmdBeginQuery},
    {"vkCmdBeginQueryIndexedEXT", &wine_vkCmdBeginQueryIndexedEXT},
    {"vkCmdBeginRenderPass", &wine_vkCmdBeginRenderPass},
    {"vkCmdBeginRenderPass2", &wine_vkCmdBeginRenderPass2},
    {"vkCmdBeginRenderPass2KHR", &wine_vkCmdBeginRenderPass2KHR},
    {"vkCmdBeginTransformFeedbackEXT", &wine_vkCmdBeginTransformFeedbackEXT},
    {"vkCmdBindDescriptorSets", &wine_vkCmdBindDescriptorSets},
    {"vkCmdBindIndexBuffer", &wine_vkCmdBindIndexBuffer},
    {"vkCmdBindPipeline", &wine_vkCmdBindPipeline},
    {"vkCmdBindPipelineShaderGroupNV", &wine_vkCmdBindPipelineShaderGroupNV},
    {"vkCmdBindShadingRateImageNV", &wine_vkCmdBindShadingRateImageNV},
    {"vkCmdBindTransformFeedbackBuffersEXT", &wine_vkCmdBindTransformFeedbackBuffersEXT},
    {"vkCmdBindVertexBuffers", &wine_vkCmdBindVertexBuffers},
    {"vkCmdBindVertexBuffers2EXT", &wine_vkCmdBindVertexBuffers2EXT},
    {"vkCmdBlitImage", &wine_vkCmdBlitImage},
    {"vkCmdBlitImage2KHR", &wine_vkCmdBlitImage2KHR},
    {"vkCmdBuildAccelerationStructureNV", &wine_vkCmdBuildAccelerationStructureNV},
    {"vkCmdBuildAccelerationStructuresIndirectKHR", &wine_vkCmdBuildAccelerationStructuresIndirectKHR},
    {"vkCmdBuildAccelerationStructuresKHR", &wine_vkCmdBuildAccelerationStructuresKHR},
    {"vkCmdClearAttachments", &wine_vkCmdClearAttachments},
    {"vkCmdClearColorImage", &wine_vkCmdClearColorImage},
    {"vkCmdClearDepthStencilImage", &wine_vkCmdClearDepthStencilImage},
    {"vkCmdCopyAccelerationStructureKHR", &wine_vkCmdCopyAccelerationStructureKHR},
    {"vkCmdCopyAccelerationStructureNV", &wine_vkCmdCopyAccelerationStructureNV},
    {"vkCmdCopyAccelerationStructureToMemoryKHR", &wine_vkCmdCopyAccelerationStructureToMemoryKHR},
    {"vkCmdCopyBuffer", &wine_vkCmdCopyBuffer},
    {"vkCmdCopyBuffer2KHR", &wine_vkCmdCopyBuffer2KHR},
    {"vkCmdCopyBufferToImage", &wine_vkCmdCopyBufferToImage},
    {"vkCmdCopyBufferToImage2KHR", &wine_vkCmdCopyBufferToImage2KHR},
    {"vkCmdCopyImage", &wine_vkCmdCopyImage},
    {"vkCmdCopyImage2KHR", &wine_vkCmdCopyImage2KHR},
    {"vkCmdCopyImageToBuffer", &wine_vkCmdCopyImageToBuffer},
    {"vkCmdCopyImageToBuffer2KHR", &wine_vkCmdCopyImageToBuffer2KHR},
    {"vkCmdCopyMemoryToAccelerationStructureKHR", &wine_vkCmdCopyMemoryToAccelerationStructureKHR},
    {"vkCmdCopyQueryPoolResults", &wine_vkCmdCopyQueryPoolResults},
    {"vkCmdDebugMarkerBeginEXT", &wine_vkCmdDebugMarkerBeginEXT},
    {"vkCmdDebugMarkerEndEXT", &wine_vkCmdDebugMarkerEndEXT},
    {"vkCmdDebugMarkerInsertEXT", &wine_vkCmdDebugMarkerInsertEXT},
    {"vkCmdDispatch", &wine_vkCmdDispatch},
    {"vkCmdDispatchBase", &wine_vkCmdDispatchBase},
    {"vkCmdDispatchBaseKHR", &wine_vkCmdDispatchBaseKHR},
    {"vkCmdDispatchIndirect", &wine_vkCmdDispatchIndirect},
    {"vkCmdDraw", &wine_vkCmdDraw},
    {"vkCmdDrawIndexed", &wine_vkCmdDrawIndexed},
    {"vkCmdDrawIndexedIndirect", &wine_vkCmdDrawIndexedIndirect},
    {"vkCmdDrawIndexedIndirectCount", &wine_vkCmdDrawIndexedIndirectCount},
    {"vkCmdDrawIndexedIndirectCountAMD", &wine_vkCmdDrawIndexedIndirectCountAMD},
    {"vkCmdDrawIndexedIndirectCountKHR", &wine_vkCmdDrawIndexedIndirectCountKHR},
    {"vkCmdDrawIndirect", &wine_vkCmdDrawIndirect},
    {"vkCmdDrawIndirectByteCountEXT", &wine_vkCmdDrawIndirectByteCountEXT},
    {"vkCmdDrawIndirectCount", &wine_vkCmdDrawIndirectCount},
    {"vkCmdDrawIndirectCountAMD", &wine_vkCmdDrawIndirectCountAMD},
    {"vkCmdDrawIndirectCountKHR", &wine_vkCmdDrawIndirectCountKHR},
    {"vkCmdDrawMeshTasksIndirectCountNV", &wine_vkCmdDrawMeshTasksIndirectCountNV},
    {"vkCmdDrawMeshTasksIndirectNV", &wine_vkCmdDrawMeshTasksIndirectNV},
    {"vkCmdDrawMeshTasksNV", &wine_vkCmdDrawMeshTasksNV},
    {"vkCmdEndConditionalRenderingEXT", &wine_vkCmdEndConditionalRenderingEXT},
    {"vkCmdEndDebugUtilsLabelEXT", &wine_vkCmdEndDebugUtilsLabelEXT},
    {"vkCmdEndQuery", &wine_vkCmdEndQuery},
    {"vkCmdEndQueryIndexedEXT", &wine_vkCmdEndQueryIndexedEXT},
    {"vkCmdEndRenderPass", &wine_vkCmdEndRenderPass},
    {"vkCmdEndRenderPass2", &wine_vkCmdEndRenderPass2},
    {"vkCmdEndRenderPass2KHR", &wine_vkCmdEndRenderPass2KHR},
    {"vkCmdEndTransformFeedbackEXT", &wine_vkCmdEndTransformFeedbackEXT},
    {"vkCmdExecuteCommands", &wine_vkCmdExecuteCommands},
    {"vkCmdExecuteGeneratedCommandsNV", &wine_vkCmdExecuteGeneratedCommandsNV},
    {"vkCmdFillBuffer", &wine_vkCmdFillBuffer},
    {"vkCmdInsertDebugUtilsLabelEXT", &wine_vkCmdInsertDebugUtilsLabelEXT},
    {"vkCmdNextSubpass", &wine_vkCmdNextSubpass},
    {"vkCmdNextSubpass2", &wine_vkCmdNextSubpass2},
    {"vkCmdNextSubpass2KHR", &wine_vkCmdNextSubpass2KHR},
    {"vkCmdPipelineBarrier", &wine_vkCmdPipelineBarrier},
    {"vkCmdPipelineBarrier2KHR", &wine_vkCmdPipelineBarrier2KHR},
    {"vkCmdPreprocessGeneratedCommandsNV", &wine_vkCmdPreprocessGeneratedCommandsNV},
    {"vkCmdPushConstants", &wine_vkCmdPushConstants},
    {"vkCmdPushDescriptorSetKHR", &wine_vkCmdPushDescriptorSetKHR},
    {"vkCmdPushDescriptorSetWithTemplateKHR", &wine_vkCmdPushDescriptorSetWithTemplateKHR},
    {"vkCmdResetEvent", &wine_vkCmdResetEvent},
    {"vkCmdResetEvent2KHR", &wine_vkCmdResetEvent2KHR},
    {"vkCmdResetQueryPool", &wine_vkCmdResetQueryPool},
    {"vkCmdResolveImage", &wine_vkCmdResolveImage},
    {"vkCmdResolveImage2KHR", &wine_vkCmdResolveImage2KHR},
    {"vkCmdSetBlendConstants", &wine_vkCmdSetBlendConstants},
    {"vkCmdSetCheckpointNV", &wine_vkCmdSetCheckpointNV},
    {"vkCmdSetCoarseSampleOrderNV", &wine_vkCmdSetCoarseSampleOrderNV},
    {"vkCmdSetColorWriteEnableEXT", &wine_vkCmdSetColorWriteEnableEXT},
    {"vkCmdSetCullModeEXT", &wine_vkCmdSetCullModeEXT},
    {"vkCmdSetDepthBias", &wine_vkCmdSetDepthBias},
    {"vkCmdSetDepthBiasEnableEXT", &wine_vkCmdSetDepthBiasEnableEXT},
    {"vkCmdSetDepthBounds", &wine_vkCmdSetDepthBounds},
    {"vkCmdSetDepthBoundsTestEnableEXT", &wine_vkCmdSetDepthBoundsTestEnableEXT},
    {"vkCmdSetDepthCompareOpEXT", &wine_vkCmdSetDepthCompareOpEXT},
    {"vkCmdSetDepthTestEnableEXT", &wine_vkCmdSetDepthTestEnableEXT},
    {"vkCmdSetDepthWriteEnableEXT", &wine_vkCmdSetDepthWriteEnableEXT},
    {"vkCmdSetDeviceMask", &wine_vkCmdSetDeviceMask},
    {"vkCmdSetDeviceMaskKHR", &wine_vkCmdSetDeviceMaskKHR},
    {"vkCmdSetDiscardRectangleEXT", &wine_vkCmdSetDiscardRectangleEXT},
    {"vkCmdSetEvent", &wine_vkCmdSetEvent},
    {"vkCmdSetEvent2KHR", &wine_vkCmdSetEvent2KHR},
    {"vkCmdSetExclusiveScissorNV", &wine_vkCmdSetExclusiveScissorNV},
    {"vkCmdSetFragmentShadingRateEnumNV", &wine_vkCmdSetFragmentShadingRateEnumNV},
    {"vkCmdSetFragmentShadingRateKHR", &wine_vkCmdSetFragmentShadingRateKHR},
    {"vkCmdSetFrontFaceEXT", &wine_vkCmdSetFrontFaceEXT},
    {"vkCmdSetLineStippleEXT", &wine_vkCmdSetLineStippleEXT},
    {"vkCmdSetLineWidth", &wine_vkCmdSetLineWidth},
    {"vkCmdSetLogicOpEXT", &wine_vkCmdSetLogicOpEXT},
    {"vkCmdSetPatchControlPointsEXT", &wine_vkCmdSetPatchControlPointsEXT},
    {"vkCmdSetPerformanceMarkerINTEL", &wine_vkCmdSetPerformanceMarkerINTEL},
    {"vkCmdSetPerformanceOverrideINTEL", &wine_vkCmdSetPerformanceOverrideINTEL},
    {"vkCmdSetPerformanceStreamMarkerINTEL", &wine_vkCmdSetPerformanceStreamMarkerINTEL},
    {"vkCmdSetPrimitiveRestartEnableEXT", &wine_vkCmdSetPrimitiveRestartEnableEXT},
    {"vkCmdSetPrimitiveTopologyEXT", &wine_vkCmdSetPrimitiveTopologyEXT},
    {"vkCmdSetRasterizerDiscardEnableEXT", &wine_vkCmdSetRasterizerDiscardEnableEXT},
    {"vkCmdSetRayTracingPipelineStackSizeKHR", &wine_vkCmdSetRayTracingPipelineStackSizeKHR},
    {"vkCmdSetSampleLocationsEXT", &wine_vkCmdSetSampleLocationsEXT},
    {"vkCmdSetScissor", &wine_vkCmdSetScissor},
    {"vkCmdSetScissorWithCountEXT", &wine_vkCmdSetScissorWithCountEXT},
    {"vkCmdSetStencilCompareMask", &wine_vkCmdSetStencilCompareMask},
    {"vkCmdSetStencilOpEXT", &wine_vkCmdSetStencilOpEXT},
    {"vkCmdSetStencilReference", &wine_vkCmdSetStencilReference},
    {"vkCmdSetStencilTestEnableEXT", &wine_vkCmdSetStencilTestEnableEXT},
    {"vkCmdSetStencilWriteMask", &wine_vkCmdSetStencilWriteMask},
    {"vkCmdSetVertexInputEXT", &wine_vkCmdSetVertexInputEXT},
    {"vkCmdSetViewport", &wine_vkCmdSetViewport},
    {"vkCmdSetViewportShadingRatePaletteNV", &wine_vkCmdSetViewportShadingRatePaletteNV},
    {"vkCmdSetViewportWScalingNV", &wine_vkCmdSetViewportWScalingNV},
    {"vkCmdSetViewportWithCountEXT", &wine_vkCmdSetViewportWithCountEXT},
    {"vkCmdTraceRaysIndirectKHR", &wine_vkCmdTraceRaysIndirectKHR},
    {"vkCmdTraceRaysKHR", &wine_vkCmdTraceRaysKHR},
    {"vkCmdTraceRaysNV", &wine_vkCmdTraceRaysNV},
    {"vkCmdUpdateBuffer", &wine_vkCmdUpdateBuffer},
    {"vkCmdWaitEvents", &wine_vkCmdWaitEvents},
    {"vkCmdWaitEvents2KHR", &wine_vkCmdWaitEvents2KHR},
    {"vkCmdWriteAccelerationStructuresPropertiesKHR", &wine_vkCmdWriteAccelerationStructuresPropertiesKHR},
    {"vkCmdWriteAccelerationStructuresPropertiesNV", &wine_vkCmdWriteAccelerationStructuresPropertiesNV},
    {"vkCmdWriteBufferMarker2AMD", &wine_vkCmdWriteBufferMarker2AMD},
    {"vkCmdWriteBufferMarkerAMD", &wine_vkCmdWriteBufferMarkerAMD},
    {"vkCmdWriteTimestamp", &wine_vkCmdWriteTimestamp},
    {"vkCmdWriteTimestamp2KHR", &wine_vkCmdWriteTimestamp2KHR},
    {"vkCompileDeferredNV", &wine_vkCompileDeferredNV},
    {"vkCopyAccelerationStructureKHR", &wine_vkCopyAccelerationStructureKHR},
    {"vkCopyAccelerationStructureToMemoryKHR", &wine_vkCopyAccelerationStructureToMemoryKHR},
    {"vkCopyMemoryToAccelerationStructureKHR", &wine_vkCopyMemoryToAccelerationStructureKHR},
    {"vkCreateAccelerationStructureKHR", &wine_vkCreateAccelerationStructureKHR},
    {"vkCreateAccelerationStructureNV", &wine_vkCreateAccelerationStructureNV},
    {"vkCreateBuffer", &wine_vkCreateBuffer},
    {"vkCreateBufferView", &wine_vkCreateBufferView},
    {"vkCreateCommandPool", &wine_vkCreateCommandPool},
    {"vkCreateComputePipelines", &wine_vkCreateComputePipelines},
    {"vkCreateDeferredOperationKHR", &wine_vkCreateDeferredOperationKHR},
    {"vkCreateDescriptorPool", &wine_vkCreateDescriptorPool},
    {"vkCreateDescriptorSetLayout", &wine_vkCreateDescriptorSetLayout},
    {"vkCreateDescriptorUpdateTemplate", &wine_vkCreateDescriptorUpdateTemplate},
    {"vkCreateDescriptorUpdateTemplateKHR", &wine_vkCreateDescriptorUpdateTemplateKHR},
    {"vkCreateEvent", &wine_vkCreateEvent},
    {"vkCreateFence", &wine_vkCreateFence},
    {"vkCreateFramebuffer", &wine_vkCreateFramebuffer},
    {"vkCreateGraphicsPipelines", &wine_vkCreateGraphicsPipelines},
    {"vkCreateImage", &wine_vkCreateImage},
    {"vkCreateImageView", &wine_vkCreateImageView},
    {"vkCreateIndirectCommandsLayoutNV", &wine_vkCreateIndirectCommandsLayoutNV},
    {"vkCreatePipelineCache", &wine_vkCreatePipelineCache},
    {"vkCreatePipelineLayout", &wine_vkCreatePipelineLayout},
    {"vkCreatePrivateDataSlotEXT", &wine_vkCreatePrivateDataSlotEXT},
    {"vkCreateQueryPool", &wine_vkCreateQueryPool},
    {"vkCreateRayTracingPipelinesKHR", &wine_vkCreateRayTracingPipelinesKHR},
    {"vkCreateRayTracingPipelinesNV", &wine_vkCreateRayTracingPipelinesNV},
    {"vkCreateRenderPass", &wine_vkCreateRenderPass},
    {"vkCreateRenderPass2", &wine_vkCreateRenderPass2},
    {"vkCreateRenderPass2KHR", &wine_vkCreateRenderPass2KHR},
    {"vkCreateSampler", &wine_vkCreateSampler},
    {"vkCreateSamplerYcbcrConversion", &wine_vkCreateSamplerYcbcrConversion},
    {"vkCreateSamplerYcbcrConversionKHR", &wine_vkCreateSamplerYcbcrConversionKHR},
    {"vkCreateSemaphore", &wine_vkCreateSemaphore},
    {"vkCreateShaderModule", &wine_vkCreateShaderModule},
    {"vkCreateSwapchainKHR", &wine_vkCreateSwapchainKHR},
    {"vkCreateValidationCacheEXT", &wine_vkCreateValidationCacheEXT},
    {"vkDebugMarkerSetObjectNameEXT", &wine_vkDebugMarkerSetObjectNameEXT},
    {"vkDebugMarkerSetObjectTagEXT", &wine_vkDebugMarkerSetObjectTagEXT},
    {"vkDeferredOperationJoinKHR", &wine_vkDeferredOperationJoinKHR},
    {"vkDestroyAccelerationStructureKHR", &wine_vkDestroyAccelerationStructureKHR},
    {"vkDestroyAccelerationStructureNV", &wine_vkDestroyAccelerationStructureNV},
    {"vkDestroyBuffer", &wine_vkDestroyBuffer},
    {"vkDestroyBufferView", &wine_vkDestroyBufferView},
    {"vkDestroyCommandPool", &wine_vkDestroyCommandPool},
    {"vkDestroyDeferredOperationKHR", &wine_vkDestroyDeferredOperationKHR},
    {"vkDestroyDescriptorPool", &wine_vkDestroyDescriptorPool},
    {"vkDestroyDescriptorSetLayout", &wine_vkDestroyDescriptorSetLayout},
    {"vkDestroyDescriptorUpdateTemplate", &wine_vkDestroyDescriptorUpdateTemplate},
    {"vkDestroyDescriptorUpdateTemplateKHR", &wine_vkDestroyDescriptorUpdateTemplateKHR},
    {"vkDestroyDevice", &wine_vkDestroyDevice},
    {"vkDestroyEvent", &wine_vkDestroyEvent},
    {"vkDestroyFence", &wine_vkDestroyFence},
    {"vkDestroyFramebuffer", &wine_vkDestroyFramebuffer},
    {"vkDestroyImage", &wine_vkDestroyImage},
    {"vkDestroyImageView", &wine_vkDestroyImageView},
    {"vkDestroyIndirectCommandsLayoutNV", &wine_vkDestroyIndirectCommandsLayoutNV},
    {"vkDestroyPipeline", &wine_vkDestroyPipeline},
    {"vkDestroyPipelineCache", &wine_vkDestroyPipelineCache},
    {"vkDestroyPipelineLayout", &wine_vkDestroyPipelineLayout},
    {"vkDestroyPrivateDataSlotEXT", &wine_vkDestroyPrivateDataSlotEXT},
    {"vkDestroyQueryPool", &wine_vkDestroyQueryPool},
    {"vkDestroyRenderPass", &wine_vkDestroyRenderPass},
    {"vkDestroySampler", &wine_vkDestroySampler},
    {"vkDestroySamplerYcbcrConversion", &wine_vkDestroySamplerYcbcrConversion},
    {"vkDestroySamplerYcbcrConversionKHR", &wine_vkDestroySamplerYcbcrConversionKHR},
    {"vkDestroySemaphore", &wine_vkDestroySemaphore},
    {"vkDestroyShaderModule", &wine_vkDestroyShaderModule},
    {"vkDestroySwapchainKHR", &wine_vkDestroySwapchainKHR},
    {"vkDestroyValidationCacheEXT", &wine_vkDestroyValidationCacheEXT},
    {"vkDeviceWaitIdle", &wine_vkDeviceWaitIdle},
    {"vkEndCommandBuffer", &wine_vkEndCommandBuffer},
    {"vkFlushMappedMemoryRanges", &wine_vkFlushMappedMemoryRanges},
    {"vkFreeCommandBuffers", &wine_vkFreeCommandBuffers},
    {"vkFreeDescriptorSets", &wine_vkFreeDescriptorSets},
    {"vkFreeMemory", &wine_vkFreeMemory},
    {"vkGetAccelerationStructureBuildSizesKHR", &wine_vkGetAccelerationStructureBuildSizesKHR},
    {"vkGetAccelerationStructureDeviceAddressKHR", &wine_vkGetAccelerationStructureDeviceAddressKHR},
    {"vkGetAccelerationStructureHandleNV", &wine_vkGetAccelerationStructureHandleNV},
    {"vkGetAccelerationStructureMemoryRequirementsNV", &wine_vkGetAccelerationStructureMemoryRequirementsNV},
    {"vkGetBufferDeviceAddress", &wine_vkGetBufferDeviceAddress},
    {"vkGetBufferDeviceAddressEXT", &wine_vkGetBufferDeviceAddressEXT},
    {"vkGetBufferDeviceAddressKHR", &wine_vkGetBufferDeviceAddressKHR},
    {"vkGetBufferMemoryRequirements", &wine_vkGetBufferMemoryRequirements},
    {"vkGetBufferMemoryRequirements2", &wine_vkGetBufferMemoryRequirements2},
    {"vkGetBufferMemoryRequirements2KHR", &wine_vkGetBufferMemoryRequirements2KHR},
    {"vkGetBufferOpaqueCaptureAddress", &wine_vkGetBufferOpaqueCaptureAddress},
    {"vkGetBufferOpaqueCaptureAddressKHR", &wine_vkGetBufferOpaqueCaptureAddressKHR},
    {"vkGetCalibratedTimestampsEXT", &wine_vkGetCalibratedTimestampsEXT},
    {"vkGetDeferredOperationMaxConcurrencyKHR", &wine_vkGetDeferredOperationMaxConcurrencyKHR},
    {"vkGetDeferredOperationResultKHR", &wine_vkGetDeferredOperationResultKHR},
    {"vkGetDescriptorSetLayoutSupport", &wine_vkGetDescriptorSetLayoutSupport},
    {"vkGetDescriptorSetLayoutSupportKHR", &wine_vkGetDescriptorSetLayoutSupportKHR},
    {"vkGetDeviceAccelerationStructureCompatibilityKHR", &wine_vkGetDeviceAccelerationStructureCompatibilityKHR},
    {"vkGetDeviceGroupPeerMemoryFeatures", &wine_vkGetDeviceGroupPeerMemoryFeatures},
    {"vkGetDeviceGroupPeerMemoryFeaturesKHR", &wine_vkGetDeviceGroupPeerMemoryFeaturesKHR},
    {"vkGetDeviceGroupPresentCapabilitiesKHR", &wine_vkGetDeviceGroupPresentCapabilitiesKHR},
    {"vkGetDeviceGroupSurfacePresentModesKHR", &wine_vkGetDeviceGroupSurfacePresentModesKHR},
    {"vkGetDeviceMemoryCommitment", &wine_vkGetDeviceMemoryCommitment},
    {"vkGetDeviceMemoryOpaqueCaptureAddress", &wine_vkGetDeviceMemoryOpaqueCaptureAddress},
    {"vkGetDeviceMemoryOpaqueCaptureAddressKHR", &wine_vkGetDeviceMemoryOpaqueCaptureAddressKHR},
    {"vkGetDeviceProcAddr", &wine_vkGetDeviceProcAddr},
    {"vkGetDeviceQueue", &wine_vkGetDeviceQueue},
    {"vkGetDeviceQueue2", &wine_vkGetDeviceQueue2},
    {"vkGetEventStatus", &wine_vkGetEventStatus},
    {"vkGetFenceStatus", &wine_vkGetFenceStatus},
    {"vkGetGeneratedCommandsMemoryRequirementsNV", &wine_vkGetGeneratedCommandsMemoryRequirementsNV},
    {"vkGetImageMemoryRequirements", &wine_vkGetImageMemoryRequirements},
    {"vkGetImageMemoryRequirements2", &wine_vkGetImageMemoryRequirements2},
    {"vkGetImageMemoryRequirements2KHR", &wine_vkGetImageMemoryRequirements2KHR},
    {"vkGetImageSparseMemoryRequirements", &wine_vkGetImageSparseMemoryRequirements},
    {"vkGetImageSparseMemoryRequirements2", &wine_vkGetImageSparseMemoryRequirements2},
    {"vkGetImageSparseMemoryRequirements2KHR", &wine_vkGetImageSparseMemoryRequirements2KHR},
    {"vkGetImageSubresourceLayout", &wine_vkGetImageSubresourceLayout},
    {"vkGetMemoryHostPointerPropertiesEXT", &wine_vkGetMemoryHostPointerPropertiesEXT},
    {"vkGetPerformanceParameterINTEL", &wine_vkGetPerformanceParameterINTEL},
    {"vkGetPipelineCacheData", &wine_vkGetPipelineCacheData},
    {"vkGetPipelineExecutableInternalRepresentationsKHR", &wine_vkGetPipelineExecutableInternalRepresentationsKHR},
    {"vkGetPipelineExecutablePropertiesKHR", &wine_vkGetPipelineExecutablePropertiesKHR},
    {"vkGetPipelineExecutableStatisticsKHR", &wine_vkGetPipelineExecutableStatisticsKHR},
    {"vkGetPrivateDataEXT", &wine_vkGetPrivateDataEXT},
    {"vkGetQueryPoolResults", &wine_vkGetQueryPoolResults},
    {"vkGetQueueCheckpointData2NV", &wine_vkGetQueueCheckpointData2NV},
    {"vkGetQueueCheckpointDataNV", &wine_vkGetQueueCheckpointDataNV},
    {"vkGetRayTracingCaptureReplayShaderGroupHandlesKHR", &wine_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR},
    {"vkGetRayTracingShaderGroupHandlesKHR", &wine_vkGetRayTracingShaderGroupHandlesKHR},
    {"vkGetRayTracingShaderGroupHandlesNV", &wine_vkGetRayTracingShaderGroupHandlesNV},
    {"vkGetRayTracingShaderGroupStackSizeKHR", &wine_vkGetRayTracingShaderGroupStackSizeKHR},
    {"vkGetRenderAreaGranularity", &wine_vkGetRenderAreaGranularity},
    {"vkGetSemaphoreCounterValue", &wine_vkGetSemaphoreCounterValue},
    {"vkGetSemaphoreCounterValueKHR", &wine_vkGetSemaphoreCounterValueKHR},
    {"vkGetShaderInfoAMD", &wine_vkGetShaderInfoAMD},
    {"vkGetSwapchainImagesKHR", &wine_vkGetSwapchainImagesKHR},
    {"vkGetValidationCacheDataEXT", &wine_vkGetValidationCacheDataEXT},
    {"vkInitializePerformanceApiINTEL", &wine_vkInitializePerformanceApiINTEL},
    {"vkInvalidateMappedMemoryRanges", &wine_vkInvalidateMappedMemoryRanges},
    {"vkMapMemory", &wine_vkMapMemory},
    {"vkMergePipelineCaches", &wine_vkMergePipelineCaches},
    {"vkMergeValidationCachesEXT", &wine_vkMergeValidationCachesEXT},
    {"vkQueueBeginDebugUtilsLabelEXT", &wine_vkQueueBeginDebugUtilsLabelEXT},
    {"vkQueueBindSparse", &wine_vkQueueBindSparse},
    {"vkQueueEndDebugUtilsLabelEXT", &wine_vkQueueEndDebugUtilsLabelEXT},
    {"vkQueueInsertDebugUtilsLabelEXT", &wine_vkQueueInsertDebugUtilsLabelEXT},
    {"vkQueuePresentKHR", &wine_vkQueuePresentKHR},
    {"vkQueueSetPerformanceConfigurationINTEL", &wine_vkQueueSetPerformanceConfigurationINTEL},
    {"vkQueueSubmit", &wine_vkQueueSubmit},
    {"vkQueueSubmit2KHR", &wine_vkQueueSubmit2KHR},
    {"vkQueueWaitIdle", &wine_vkQueueWaitIdle},
    {"vkReleasePerformanceConfigurationINTEL", &wine_vkReleasePerformanceConfigurationINTEL},
    {"vkReleaseProfilingLockKHR", &wine_vkReleaseProfilingLockKHR},
    {"vkResetCommandBuffer", &wine_vkResetCommandBuffer},
    {"vkResetCommandPool", &wine_vkResetCommandPool},
    {"vkResetDescriptorPool", &wine_vkResetDescriptorPool},
    {"vkResetEvent", &wine_vkResetEvent},
    {"vkResetFences", &wine_vkResetFences},
    {"vkResetQueryPool", &wine_vkResetQueryPool},
    {"vkResetQueryPoolEXT", &wine_vkResetQueryPoolEXT},
    {"vkSetDebugUtilsObjectNameEXT", &wine_vkSetDebugUtilsObjectNameEXT},
    {"vkSetDebugUtilsObjectTagEXT", &wine_vkSetDebugUtilsObjectTagEXT},
    {"vkSetEvent", &wine_vkSetEvent},
    {"vkSetPrivateDataEXT", &wine_vkSetPrivateDataEXT},
    {"vkSignalSemaphore", &wine_vkSignalSemaphore},
    {"vkSignalSemaphoreKHR", &wine_vkSignalSemaphoreKHR},
    {"vkTrimCommandPool", &wine_vkTrimCommandPool},
    {"vkTrimCommandPoolKHR", &wine_vkTrimCommandPoolKHR},
    {"vkUninitializePerformanceApiINTEL", &wine_vkUninitializePerformanceApiINTEL},
    {"vkUnmapMemory", &wine_vkUnmapMemory},
    {"vkUpdateDescriptorSetWithTemplate", &wine_vkUpdateDescriptorSetWithTemplate},
    {"vkUpdateDescriptorSetWithTemplateKHR", &wine_vkUpdateDescriptorSetWithTemplateKHR},
    {"vkUpdateDescriptorSets", &wine_vkUpdateDescriptorSets},
    {"vkWaitForFences", &wine_vkWaitForFences},
    {"vkWaitSemaphores", &wine_vkWaitSemaphores},
    {"vkWaitSemaphoresKHR", &wine_vkWaitSemaphoresKHR},
    {"vkWriteAccelerationStructuresPropertiesKHR", &wine_vkWriteAccelerationStructuresPropertiesKHR},
};

static const struct vulkan_func vk_phys_dev_dispatch_table[] =
{
    {"vkCreateDevice", &wine_vkCreateDevice},
    {"vkEnumerateDeviceExtensionProperties", &wine_vkEnumerateDeviceExtensionProperties},
    {"vkEnumerateDeviceLayerProperties", &wine_vkEnumerateDeviceLayerProperties},
    {"vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR", &wine_vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR},
    {"vkGetPhysicalDeviceCalibrateableTimeDomainsEXT", &wine_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT},
    {"vkGetPhysicalDeviceCooperativeMatrixPropertiesNV", &wine_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV},
    {"vkGetPhysicalDeviceExternalBufferProperties", &wine_vkGetPhysicalDeviceExternalBufferProperties},
    {"vkGetPhysicalDeviceExternalBufferPropertiesKHR", &wine_vkGetPhysicalDeviceExternalBufferPropertiesKHR},
    {"vkGetPhysicalDeviceExternalFenceProperties", &wine_vkGetPhysicalDeviceExternalFenceProperties},
    {"vkGetPhysicalDeviceExternalFencePropertiesKHR", &wine_vkGetPhysicalDeviceExternalFencePropertiesKHR},
    {"vkGetPhysicalDeviceExternalSemaphoreProperties", &wine_vkGetPhysicalDeviceExternalSemaphoreProperties},
    {"vkGetPhysicalDeviceExternalSemaphorePropertiesKHR", &wine_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR},
    {"vkGetPhysicalDeviceFeatures", &wine_vkGetPhysicalDeviceFeatures},
    {"vkGetPhysicalDeviceFeatures2", &wine_vkGetPhysicalDeviceFeatures2},
    {"vkGetPhysicalDeviceFeatures2KHR", &wine_vkGetPhysicalDeviceFeatures2KHR},
    {"vkGetPhysicalDeviceFormatProperties", &wine_vkGetPhysicalDeviceFormatProperties},
    {"vkGetPhysicalDeviceFormatProperties2", &wine_vkGetPhysicalDeviceFormatProperties2},
    {"vkGetPhysicalDeviceFormatProperties2KHR", &wine_vkGetPhysicalDeviceFormatProperties2KHR},
    {"vkGetPhysicalDeviceFragmentShadingRatesKHR", &wine_vkGetPhysicalDeviceFragmentShadingRatesKHR},
    {"vkGetPhysicalDeviceImageFormatProperties", &wine_vkGetPhysicalDeviceImageFormatProperties},
    {"vkGetPhysicalDeviceImageFormatProperties2", &wine_vkGetPhysicalDeviceImageFormatProperties2},
    {"vkGetPhysicalDeviceImageFormatProperties2KHR", &wine_vkGetPhysicalDeviceImageFormatProperties2KHR},
    {"vkGetPhysicalDeviceMemoryProperties", &wine_vkGetPhysicalDeviceMemoryProperties},
    {"vkGetPhysicalDeviceMemoryProperties2", &wine_vkGetPhysicalDeviceMemoryProperties2},
    {"vkGetPhysicalDeviceMemoryProperties2KHR", &wine_vkGetPhysicalDeviceMemoryProperties2KHR},
    {"vkGetPhysicalDeviceMultisamplePropertiesEXT", &wine_vkGetPhysicalDeviceMultisamplePropertiesEXT},
    {"vkGetPhysicalDevicePresentRectanglesKHR", &wine_vkGetPhysicalDevicePresentRectanglesKHR},
    {"vkGetPhysicalDeviceProperties", &wine_vkGetPhysicalDeviceProperties},
    {"vkGetPhysicalDeviceProperties2", &wine_vkGetPhysicalDeviceProperties2},
    {"vkGetPhysicalDeviceProperties2KHR", &wine_vkGetPhysicalDeviceProperties2KHR},
    {"vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR", &wine_vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR},
    {"vkGetPhysicalDeviceQueueFamilyProperties", &wine_vkGetPhysicalDeviceQueueFamilyProperties},
    {"vkGetPhysicalDeviceQueueFamilyProperties2", &wine_vkGetPhysicalDeviceQueueFamilyProperties2},
    {"vkGetPhysicalDeviceQueueFamilyProperties2KHR", &wine_vkGetPhysicalDeviceQueueFamilyProperties2KHR},
    {"vkGetPhysicalDeviceSparseImageFormatProperties", &wine_vkGetPhysicalDeviceSparseImageFormatProperties},
    {"vkGetPhysicalDeviceSparseImageFormatProperties2", &wine_vkGetPhysicalDeviceSparseImageFormatProperties2},
    {"vkGetPhysicalDeviceSparseImageFormatProperties2KHR", &wine_vkGetPhysicalDeviceSparseImageFormatProperties2KHR},
    {"vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV", &wine_vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV},
    {"vkGetPhysicalDeviceSurfaceCapabilities2KHR", &wine_vkGetPhysicalDeviceSurfaceCapabilities2KHR},
    {"vkGetPhysicalDeviceSurfaceCapabilitiesKHR", &wine_vkGetPhysicalDeviceSurfaceCapabilitiesKHR},
    {"vkGetPhysicalDeviceSurfaceFormats2KHR", &wine_vkGetPhysicalDeviceSurfaceFormats2KHR},
    {"vkGetPhysicalDeviceSurfaceFormatsKHR", &wine_vkGetPhysicalDeviceSurfaceFormatsKHR},
    {"vkGetPhysicalDeviceSurfacePresentModesKHR", &wine_vkGetPhysicalDeviceSurfacePresentModesKHR},
    {"vkGetPhysicalDeviceSurfaceSupportKHR", &wine_vkGetPhysicalDeviceSurfaceSupportKHR},
    {"vkGetPhysicalDeviceToolPropertiesEXT", &wine_vkGetPhysicalDeviceToolPropertiesEXT},
    {"vkGetPhysicalDeviceWin32PresentationSupportKHR", &wine_vkGetPhysicalDeviceWin32PresentationSupportKHR},
};

static const struct vulkan_func vk_instance_dispatch_table[] =
{
    {"vkCreateDebugReportCallbackEXT", &wine_vkCreateDebugReportCallbackEXT},
    {"vkCreateDebugUtilsMessengerEXT", &wine_vkCreateDebugUtilsMessengerEXT},
    {"vkCreateWin32SurfaceKHR", &wine_vkCreateWin32SurfaceKHR},
    {"vkDebugReportMessageEXT", &wine_vkDebugReportMessageEXT},
    {"vkDestroyDebugReportCallbackEXT", &wine_vkDestroyDebugReportCallbackEXT},
    {"vkDestroyDebugUtilsMessengerEXT", &wine_vkDestroyDebugUtilsMessengerEXT},
    {"vkDestroyInstance", &wine_vkDestroyInstance},
    {"vkDestroySurfaceKHR", &wine_vkDestroySurfaceKHR},
    {"vkEnumeratePhysicalDeviceGroups", &wine_vkEnumeratePhysicalDeviceGroups},
    {"vkEnumeratePhysicalDeviceGroupsKHR", &wine_vkEnumeratePhysicalDeviceGroupsKHR},
    {"vkEnumeratePhysicalDevices", &wine_vkEnumeratePhysicalDevices},
    {"vkSubmitDebugUtilsMessageEXT", &wine_vkSubmitDebugUtilsMessageEXT},
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
