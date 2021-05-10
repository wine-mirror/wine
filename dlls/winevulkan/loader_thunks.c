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

VkResult WINAPI vkAcquireNextImage2KHR(VkDevice device, const VkAcquireNextImageInfoKHR *pAcquireInfo, uint32_t *pImageIndex)
{
    return unix_funcs->p_vkAcquireNextImage2KHR(device, pAcquireInfo, pImageIndex);
}

VkResult WINAPI vkAcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t *pImageIndex)
{
    return unix_funcs->p_vkAcquireNextImageKHR(device, swapchain, timeout, semaphore, fence, pImageIndex);
}

VkResult WINAPI vkAcquirePerformanceConfigurationINTEL(VkDevice device, const VkPerformanceConfigurationAcquireInfoINTEL *pAcquireInfo, VkPerformanceConfigurationINTEL *pConfiguration)
{
    return unix_funcs->p_vkAcquirePerformanceConfigurationINTEL(device, pAcquireInfo, pConfiguration);
}

VkResult WINAPI vkAcquireProfilingLockKHR(VkDevice device, const VkAcquireProfilingLockInfoKHR *pInfo)
{
    return unix_funcs->p_vkAcquireProfilingLockKHR(device, pInfo);
}

VkResult WINAPI vkAllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo *pAllocateInfo, VkCommandBuffer *pCommandBuffers)
{
    return unix_funcs->p_vkAllocateCommandBuffers(device, pAllocateInfo, pCommandBuffers);
}

VkResult WINAPI vkAllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo *pAllocateInfo, VkDescriptorSet *pDescriptorSets)
{
    return unix_funcs->p_vkAllocateDescriptorSets(device, pAllocateInfo, pDescriptorSets);
}

VkResult WINAPI vkAllocateMemory(VkDevice device, const VkMemoryAllocateInfo *pAllocateInfo, const VkAllocationCallbacks *pAllocator, VkDeviceMemory *pMemory)
{
    return unix_funcs->p_vkAllocateMemory(device, pAllocateInfo, pAllocator, pMemory);
}

VkResult WINAPI vkBeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo *pBeginInfo)
{
    return unix_funcs->p_vkBeginCommandBuffer(commandBuffer, pBeginInfo);
}

VkResult WINAPI vkBindAccelerationStructureMemoryNV(VkDevice device, uint32_t bindInfoCount, const VkBindAccelerationStructureMemoryInfoNV *pBindInfos)
{
    return unix_funcs->p_vkBindAccelerationStructureMemoryNV(device, bindInfoCount, pBindInfos);
}

VkResult WINAPI vkBindBufferMemory(VkDevice device, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset)
{
    return unix_funcs->p_vkBindBufferMemory(device, buffer, memory, memoryOffset);
}

VkResult WINAPI vkBindBufferMemory2(VkDevice device, uint32_t bindInfoCount, const VkBindBufferMemoryInfo *pBindInfos)
{
    return unix_funcs->p_vkBindBufferMemory2(device, bindInfoCount, pBindInfos);
}

VkResult WINAPI vkBindBufferMemory2KHR(VkDevice device, uint32_t bindInfoCount, const VkBindBufferMemoryInfo *pBindInfos)
{
    return unix_funcs->p_vkBindBufferMemory2KHR(device, bindInfoCount, pBindInfos);
}

VkResult WINAPI vkBindImageMemory(VkDevice device, VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset)
{
    return unix_funcs->p_vkBindImageMemory(device, image, memory, memoryOffset);
}

VkResult WINAPI vkBindImageMemory2(VkDevice device, uint32_t bindInfoCount, const VkBindImageMemoryInfo *pBindInfos)
{
    return unix_funcs->p_vkBindImageMemory2(device, bindInfoCount, pBindInfos);
}

VkResult WINAPI vkBindImageMemory2KHR(VkDevice device, uint32_t bindInfoCount, const VkBindImageMemoryInfo *pBindInfos)
{
    return unix_funcs->p_vkBindImageMemory2KHR(device, bindInfoCount, pBindInfos);
}

VkResult WINAPI vkBuildAccelerationStructuresKHR(VkDevice device, VkDeferredOperationKHR deferredOperation, uint32_t infoCount, const VkAccelerationStructureBuildGeometryInfoKHR *pInfos, const VkAccelerationStructureBuildRangeInfoKHR * const*ppBuildRangeInfos)
{
    return unix_funcs->p_vkBuildAccelerationStructuresKHR(device, deferredOperation, infoCount, pInfos, ppBuildRangeInfos);
}

void WINAPI vkCmdBeginConditionalRenderingEXT(VkCommandBuffer commandBuffer, const VkConditionalRenderingBeginInfoEXT *pConditionalRenderingBegin)
{
    unix_funcs->p_vkCmdBeginConditionalRenderingEXT(commandBuffer, pConditionalRenderingBegin);
}

void WINAPI vkCmdBeginDebugUtilsLabelEXT(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT *pLabelInfo)
{
    unix_funcs->p_vkCmdBeginDebugUtilsLabelEXT(commandBuffer, pLabelInfo);
}

void WINAPI vkCmdBeginQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags)
{
    unix_funcs->p_vkCmdBeginQuery(commandBuffer, queryPool, query, flags);
}

void WINAPI vkCmdBeginQueryIndexedEXT(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags, uint32_t index)
{
    unix_funcs->p_vkCmdBeginQueryIndexedEXT(commandBuffer, queryPool, query, flags, index);
}

void WINAPI vkCmdBeginRenderPass(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo *pRenderPassBegin, VkSubpassContents contents)
{
    unix_funcs->p_vkCmdBeginRenderPass(commandBuffer, pRenderPassBegin, contents);
}

void WINAPI vkCmdBeginRenderPass2(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo *pRenderPassBegin, const VkSubpassBeginInfo *pSubpassBeginInfo)
{
    unix_funcs->p_vkCmdBeginRenderPass2(commandBuffer, pRenderPassBegin, pSubpassBeginInfo);
}

void WINAPI vkCmdBeginRenderPass2KHR(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo *pRenderPassBegin, const VkSubpassBeginInfo *pSubpassBeginInfo)
{
    unix_funcs->p_vkCmdBeginRenderPass2KHR(commandBuffer, pRenderPassBegin, pSubpassBeginInfo);
}

void WINAPI vkCmdBeginTransformFeedbackEXT(VkCommandBuffer commandBuffer, uint32_t firstCounterBuffer, uint32_t counterBufferCount, const VkBuffer *pCounterBuffers, const VkDeviceSize *pCounterBufferOffsets)
{
    unix_funcs->p_vkCmdBeginTransformFeedbackEXT(commandBuffer, firstCounterBuffer, counterBufferCount, pCounterBuffers, pCounterBufferOffsets);
}

void WINAPI vkCmdBindDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount, const VkDescriptorSet *pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t *pDynamicOffsets)
{
    unix_funcs->p_vkCmdBindDescriptorSets(commandBuffer, pipelineBindPoint, layout, firstSet, descriptorSetCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
}

void WINAPI vkCmdBindIndexBuffer(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType)
{
    unix_funcs->p_vkCmdBindIndexBuffer(commandBuffer, buffer, offset, indexType);
}

void WINAPI vkCmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline)
{
    unix_funcs->p_vkCmdBindPipeline(commandBuffer, pipelineBindPoint, pipeline);
}

void WINAPI vkCmdBindPipelineShaderGroupNV(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline, uint32_t groupIndex)
{
    unix_funcs->p_vkCmdBindPipelineShaderGroupNV(commandBuffer, pipelineBindPoint, pipeline, groupIndex);
}

void WINAPI vkCmdBindShadingRateImageNV(VkCommandBuffer commandBuffer, VkImageView imageView, VkImageLayout imageLayout)
{
    unix_funcs->p_vkCmdBindShadingRateImageNV(commandBuffer, imageView, imageLayout);
}

void WINAPI vkCmdBindTransformFeedbackBuffersEXT(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer *pBuffers, const VkDeviceSize *pOffsets, const VkDeviceSize *pSizes)
{
    unix_funcs->p_vkCmdBindTransformFeedbackBuffersEXT(commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets, pSizes);
}

void WINAPI vkCmdBindVertexBuffers(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer *pBuffers, const VkDeviceSize *pOffsets)
{
    unix_funcs->p_vkCmdBindVertexBuffers(commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets);
}

void WINAPI vkCmdBindVertexBuffers2EXT(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer *pBuffers, const VkDeviceSize *pOffsets, const VkDeviceSize *pSizes, const VkDeviceSize *pStrides)
{
    unix_funcs->p_vkCmdBindVertexBuffers2EXT(commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets, pSizes, pStrides);
}

void WINAPI vkCmdBlitImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit *pRegions, VkFilter filter)
{
    unix_funcs->p_vkCmdBlitImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions, filter);
}

void WINAPI vkCmdBlitImage2KHR(VkCommandBuffer commandBuffer, const VkBlitImageInfo2KHR *pBlitImageInfo)
{
    unix_funcs->p_vkCmdBlitImage2KHR(commandBuffer, pBlitImageInfo);
}

void WINAPI vkCmdBuildAccelerationStructureNV(VkCommandBuffer commandBuffer, const VkAccelerationStructureInfoNV *pInfo, VkBuffer instanceData, VkDeviceSize instanceOffset, VkBool32 update, VkAccelerationStructureNV dst, VkAccelerationStructureNV src, VkBuffer scratch, VkDeviceSize scratchOffset)
{
    unix_funcs->p_vkCmdBuildAccelerationStructureNV(commandBuffer, pInfo, instanceData, instanceOffset, update, dst, src, scratch, scratchOffset);
}

void WINAPI vkCmdBuildAccelerationStructuresIndirectKHR(VkCommandBuffer commandBuffer, uint32_t infoCount, const VkAccelerationStructureBuildGeometryInfoKHR *pInfos, const VkDeviceAddress *pIndirectDeviceAddresses, const uint32_t *pIndirectStrides, const uint32_t * const*ppMaxPrimitiveCounts)
{
    unix_funcs->p_vkCmdBuildAccelerationStructuresIndirectKHR(commandBuffer, infoCount, pInfos, pIndirectDeviceAddresses, pIndirectStrides, ppMaxPrimitiveCounts);
}

void WINAPI vkCmdBuildAccelerationStructuresKHR(VkCommandBuffer commandBuffer, uint32_t infoCount, const VkAccelerationStructureBuildGeometryInfoKHR *pInfos, const VkAccelerationStructureBuildRangeInfoKHR * const*ppBuildRangeInfos)
{
    unix_funcs->p_vkCmdBuildAccelerationStructuresKHR(commandBuffer, infoCount, pInfos, ppBuildRangeInfos);
}

void WINAPI vkCmdClearAttachments(VkCommandBuffer commandBuffer, uint32_t attachmentCount, const VkClearAttachment *pAttachments, uint32_t rectCount, const VkClearRect *pRects)
{
    unix_funcs->p_vkCmdClearAttachments(commandBuffer, attachmentCount, pAttachments, rectCount, pRects);
}

void WINAPI vkCmdClearColorImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, const VkClearColorValue *pColor, uint32_t rangeCount, const VkImageSubresourceRange *pRanges)
{
    unix_funcs->p_vkCmdClearColorImage(commandBuffer, image, imageLayout, pColor, rangeCount, pRanges);
}

void WINAPI vkCmdClearDepthStencilImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, const VkClearDepthStencilValue *pDepthStencil, uint32_t rangeCount, const VkImageSubresourceRange *pRanges)
{
    unix_funcs->p_vkCmdClearDepthStencilImage(commandBuffer, image, imageLayout, pDepthStencil, rangeCount, pRanges);
}

void WINAPI vkCmdCopyAccelerationStructureKHR(VkCommandBuffer commandBuffer, const VkCopyAccelerationStructureInfoKHR *pInfo)
{
    unix_funcs->p_vkCmdCopyAccelerationStructureKHR(commandBuffer, pInfo);
}

void WINAPI vkCmdCopyAccelerationStructureNV(VkCommandBuffer commandBuffer, VkAccelerationStructureNV dst, VkAccelerationStructureNV src, VkCopyAccelerationStructureModeKHR mode)
{
    unix_funcs->p_vkCmdCopyAccelerationStructureNV(commandBuffer, dst, src, mode);
}

void WINAPI vkCmdCopyAccelerationStructureToMemoryKHR(VkCommandBuffer commandBuffer, const VkCopyAccelerationStructureToMemoryInfoKHR *pInfo)
{
    unix_funcs->p_vkCmdCopyAccelerationStructureToMemoryKHR(commandBuffer, pInfo);
}

void WINAPI vkCmdCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy *pRegions)
{
    unix_funcs->p_vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, regionCount, pRegions);
}

void WINAPI vkCmdCopyBuffer2KHR(VkCommandBuffer commandBuffer, const VkCopyBufferInfo2KHR *pCopyBufferInfo)
{
    unix_funcs->p_vkCmdCopyBuffer2KHR(commandBuffer, pCopyBufferInfo);
}

void WINAPI vkCmdCopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy *pRegions)
{
    unix_funcs->p_vkCmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions);
}

void WINAPI vkCmdCopyBufferToImage2KHR(VkCommandBuffer commandBuffer, const VkCopyBufferToImageInfo2KHR *pCopyBufferToImageInfo)
{
    unix_funcs->p_vkCmdCopyBufferToImage2KHR(commandBuffer, pCopyBufferToImageInfo);
}

void WINAPI vkCmdCopyImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageCopy *pRegions)
{
    unix_funcs->p_vkCmdCopyImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
}

void WINAPI vkCmdCopyImage2KHR(VkCommandBuffer commandBuffer, const VkCopyImageInfo2KHR *pCopyImageInfo)
{
    unix_funcs->p_vkCmdCopyImage2KHR(commandBuffer, pCopyImageInfo);
}

void WINAPI vkCmdCopyImageToBuffer(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferImageCopy *pRegions)
{
    unix_funcs->p_vkCmdCopyImageToBuffer(commandBuffer, srcImage, srcImageLayout, dstBuffer, regionCount, pRegions);
}

void WINAPI vkCmdCopyImageToBuffer2KHR(VkCommandBuffer commandBuffer, const VkCopyImageToBufferInfo2KHR *pCopyImageToBufferInfo)
{
    unix_funcs->p_vkCmdCopyImageToBuffer2KHR(commandBuffer, pCopyImageToBufferInfo);
}

void WINAPI vkCmdCopyMemoryToAccelerationStructureKHR(VkCommandBuffer commandBuffer, const VkCopyMemoryToAccelerationStructureInfoKHR *pInfo)
{
    unix_funcs->p_vkCmdCopyMemoryToAccelerationStructureKHR(commandBuffer, pInfo);
}

void WINAPI vkCmdCopyQueryPoolResults(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize stride, VkQueryResultFlags flags)
{
    unix_funcs->p_vkCmdCopyQueryPoolResults(commandBuffer, queryPool, firstQuery, queryCount, dstBuffer, dstOffset, stride, flags);
}

void WINAPI vkCmdCuLaunchKernelNVX(VkCommandBuffer commandBuffer, const VkCuLaunchInfoNVX *pLaunchInfo)
{
    unix_funcs->p_vkCmdCuLaunchKernelNVX(commandBuffer, pLaunchInfo);
}

void WINAPI vkCmdDebugMarkerBeginEXT(VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT *pMarkerInfo)
{
    unix_funcs->p_vkCmdDebugMarkerBeginEXT(commandBuffer, pMarkerInfo);
}

void WINAPI vkCmdDebugMarkerEndEXT(VkCommandBuffer commandBuffer)
{
    unix_funcs->p_vkCmdDebugMarkerEndEXT(commandBuffer);
}

void WINAPI vkCmdDebugMarkerInsertEXT(VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT *pMarkerInfo)
{
    unix_funcs->p_vkCmdDebugMarkerInsertEXT(commandBuffer, pMarkerInfo);
}

void WINAPI vkCmdDispatch(VkCommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    unix_funcs->p_vkCmdDispatch(commandBuffer, groupCountX, groupCountY, groupCountZ);
}

void WINAPI vkCmdDispatchBase(VkCommandBuffer commandBuffer, uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    unix_funcs->p_vkCmdDispatchBase(commandBuffer, baseGroupX, baseGroupY, baseGroupZ, groupCountX, groupCountY, groupCountZ);
}

void WINAPI vkCmdDispatchBaseKHR(VkCommandBuffer commandBuffer, uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    unix_funcs->p_vkCmdDispatchBaseKHR(commandBuffer, baseGroupX, baseGroupY, baseGroupZ, groupCountX, groupCountY, groupCountZ);
}

void WINAPI vkCmdDispatchIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset)
{
    unix_funcs->p_vkCmdDispatchIndirect(commandBuffer, buffer, offset);
}

void WINAPI vkCmdDraw(VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    unix_funcs->p_vkCmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void WINAPI vkCmdDrawIndexed(VkCommandBuffer commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
{
    unix_funcs->p_vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void WINAPI vkCmdDrawIndexedIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
{
    unix_funcs->p_vkCmdDrawIndexedIndirect(commandBuffer, buffer, offset, drawCount, stride);
}

void WINAPI vkCmdDrawIndexedIndirectCount(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
    unix_funcs->p_vkCmdDrawIndexedIndirectCount(commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}

void WINAPI vkCmdDrawIndexedIndirectCountAMD(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
    unix_funcs->p_vkCmdDrawIndexedIndirectCountAMD(commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}

void WINAPI vkCmdDrawIndexedIndirectCountKHR(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
    unix_funcs->p_vkCmdDrawIndexedIndirectCountKHR(commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}

void WINAPI vkCmdDrawIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
{
    unix_funcs->p_vkCmdDrawIndirect(commandBuffer, buffer, offset, drawCount, stride);
}

void WINAPI vkCmdDrawIndirectByteCountEXT(VkCommandBuffer commandBuffer, uint32_t instanceCount, uint32_t firstInstance, VkBuffer counterBuffer, VkDeviceSize counterBufferOffset, uint32_t counterOffset, uint32_t vertexStride)
{
    unix_funcs->p_vkCmdDrawIndirectByteCountEXT(commandBuffer, instanceCount, firstInstance, counterBuffer, counterBufferOffset, counterOffset, vertexStride);
}

void WINAPI vkCmdDrawIndirectCount(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
    unix_funcs->p_vkCmdDrawIndirectCount(commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}

void WINAPI vkCmdDrawIndirectCountAMD(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
    unix_funcs->p_vkCmdDrawIndirectCountAMD(commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}

void WINAPI vkCmdDrawIndirectCountKHR(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
    unix_funcs->p_vkCmdDrawIndirectCountKHR(commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}

void WINAPI vkCmdDrawMeshTasksIndirectCountNV(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
    unix_funcs->p_vkCmdDrawMeshTasksIndirectCountNV(commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}

void WINAPI vkCmdDrawMeshTasksIndirectNV(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
{
    unix_funcs->p_vkCmdDrawMeshTasksIndirectNV(commandBuffer, buffer, offset, drawCount, stride);
}

void WINAPI vkCmdDrawMeshTasksNV(VkCommandBuffer commandBuffer, uint32_t taskCount, uint32_t firstTask)
{
    unix_funcs->p_vkCmdDrawMeshTasksNV(commandBuffer, taskCount, firstTask);
}

void WINAPI vkCmdEndConditionalRenderingEXT(VkCommandBuffer commandBuffer)
{
    unix_funcs->p_vkCmdEndConditionalRenderingEXT(commandBuffer);
}

void WINAPI vkCmdEndDebugUtilsLabelEXT(VkCommandBuffer commandBuffer)
{
    unix_funcs->p_vkCmdEndDebugUtilsLabelEXT(commandBuffer);
}

void WINAPI vkCmdEndQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query)
{
    unix_funcs->p_vkCmdEndQuery(commandBuffer, queryPool, query);
}

void WINAPI vkCmdEndQueryIndexedEXT(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query, uint32_t index)
{
    unix_funcs->p_vkCmdEndQueryIndexedEXT(commandBuffer, queryPool, query, index);
}

void WINAPI vkCmdEndRenderPass(VkCommandBuffer commandBuffer)
{
    unix_funcs->p_vkCmdEndRenderPass(commandBuffer);
}

void WINAPI vkCmdEndRenderPass2(VkCommandBuffer commandBuffer, const VkSubpassEndInfo *pSubpassEndInfo)
{
    unix_funcs->p_vkCmdEndRenderPass2(commandBuffer, pSubpassEndInfo);
}

void WINAPI vkCmdEndRenderPass2KHR(VkCommandBuffer commandBuffer, const VkSubpassEndInfo *pSubpassEndInfo)
{
    unix_funcs->p_vkCmdEndRenderPass2KHR(commandBuffer, pSubpassEndInfo);
}

void WINAPI vkCmdEndTransformFeedbackEXT(VkCommandBuffer commandBuffer, uint32_t firstCounterBuffer, uint32_t counterBufferCount, const VkBuffer *pCounterBuffers, const VkDeviceSize *pCounterBufferOffsets)
{
    unix_funcs->p_vkCmdEndTransformFeedbackEXT(commandBuffer, firstCounterBuffer, counterBufferCount, pCounterBuffers, pCounterBufferOffsets);
}

void WINAPI vkCmdExecuteCommands(VkCommandBuffer commandBuffer, uint32_t commandBufferCount, const VkCommandBuffer *pCommandBuffers)
{
    unix_funcs->p_vkCmdExecuteCommands(commandBuffer, commandBufferCount, pCommandBuffers);
}

void WINAPI vkCmdExecuteGeneratedCommandsNV(VkCommandBuffer commandBuffer, VkBool32 isPreprocessed, const VkGeneratedCommandsInfoNV *pGeneratedCommandsInfo)
{
    unix_funcs->p_vkCmdExecuteGeneratedCommandsNV(commandBuffer, isPreprocessed, pGeneratedCommandsInfo);
}

void WINAPI vkCmdFillBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data)
{
    unix_funcs->p_vkCmdFillBuffer(commandBuffer, dstBuffer, dstOffset, size, data);
}

void WINAPI vkCmdInsertDebugUtilsLabelEXT(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT *pLabelInfo)
{
    unix_funcs->p_vkCmdInsertDebugUtilsLabelEXT(commandBuffer, pLabelInfo);
}

void WINAPI vkCmdNextSubpass(VkCommandBuffer commandBuffer, VkSubpassContents contents)
{
    unix_funcs->p_vkCmdNextSubpass(commandBuffer, contents);
}

void WINAPI vkCmdNextSubpass2(VkCommandBuffer commandBuffer, const VkSubpassBeginInfo *pSubpassBeginInfo, const VkSubpassEndInfo *pSubpassEndInfo)
{
    unix_funcs->p_vkCmdNextSubpass2(commandBuffer, pSubpassBeginInfo, pSubpassEndInfo);
}

void WINAPI vkCmdNextSubpass2KHR(VkCommandBuffer commandBuffer, const VkSubpassBeginInfo *pSubpassBeginInfo, const VkSubpassEndInfo *pSubpassEndInfo)
{
    unix_funcs->p_vkCmdNextSubpass2KHR(commandBuffer, pSubpassBeginInfo, pSubpassEndInfo);
}

void WINAPI vkCmdPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier *pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier *pImageMemoryBarriers)
{
    unix_funcs->p_vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
}

void WINAPI vkCmdPipelineBarrier2KHR(VkCommandBuffer commandBuffer, const VkDependencyInfoKHR *pDependencyInfo)
{
    unix_funcs->p_vkCmdPipelineBarrier2KHR(commandBuffer, pDependencyInfo);
}

void WINAPI vkCmdPreprocessGeneratedCommandsNV(VkCommandBuffer commandBuffer, const VkGeneratedCommandsInfoNV *pGeneratedCommandsInfo)
{
    unix_funcs->p_vkCmdPreprocessGeneratedCommandsNV(commandBuffer, pGeneratedCommandsInfo);
}

void WINAPI vkCmdPushConstants(VkCommandBuffer commandBuffer, VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void *pValues)
{
    unix_funcs->p_vkCmdPushConstants(commandBuffer, layout, stageFlags, offset, size, pValues);
}

void WINAPI vkCmdPushDescriptorSetKHR(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t set, uint32_t descriptorWriteCount, const VkWriteDescriptorSet *pDescriptorWrites)
{
    unix_funcs->p_vkCmdPushDescriptorSetKHR(commandBuffer, pipelineBindPoint, layout, set, descriptorWriteCount, pDescriptorWrites);
}

void WINAPI vkCmdPushDescriptorSetWithTemplateKHR(VkCommandBuffer commandBuffer, VkDescriptorUpdateTemplate descriptorUpdateTemplate, VkPipelineLayout layout, uint32_t set, const void *pData)
{
    unix_funcs->p_vkCmdPushDescriptorSetWithTemplateKHR(commandBuffer, descriptorUpdateTemplate, layout, set, pData);
}

void WINAPI vkCmdResetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask)
{
    unix_funcs->p_vkCmdResetEvent(commandBuffer, event, stageMask);
}

void WINAPI vkCmdResetEvent2KHR(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags2KHR stageMask)
{
    unix_funcs->p_vkCmdResetEvent2KHR(commandBuffer, event, stageMask);
}

void WINAPI vkCmdResetQueryPool(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount)
{
    unix_funcs->p_vkCmdResetQueryPool(commandBuffer, queryPool, firstQuery, queryCount);
}

void WINAPI vkCmdResolveImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageResolve *pRegions)
{
    unix_funcs->p_vkCmdResolveImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
}

void WINAPI vkCmdResolveImage2KHR(VkCommandBuffer commandBuffer, const VkResolveImageInfo2KHR *pResolveImageInfo)
{
    unix_funcs->p_vkCmdResolveImage2KHR(commandBuffer, pResolveImageInfo);
}

void WINAPI vkCmdSetBlendConstants(VkCommandBuffer commandBuffer, const float blendConstants[4])
{
    unix_funcs->p_vkCmdSetBlendConstants(commandBuffer, blendConstants);
}

void WINAPI vkCmdSetCheckpointNV(VkCommandBuffer commandBuffer, const void *pCheckpointMarker)
{
    unix_funcs->p_vkCmdSetCheckpointNV(commandBuffer, pCheckpointMarker);
}

void WINAPI vkCmdSetCoarseSampleOrderNV(VkCommandBuffer commandBuffer, VkCoarseSampleOrderTypeNV sampleOrderType, uint32_t customSampleOrderCount, const VkCoarseSampleOrderCustomNV *pCustomSampleOrders)
{
    unix_funcs->p_vkCmdSetCoarseSampleOrderNV(commandBuffer, sampleOrderType, customSampleOrderCount, pCustomSampleOrders);
}

void WINAPI vkCmdSetColorWriteEnableEXT(VkCommandBuffer commandBuffer, uint32_t attachmentCount, const VkBool32 *pColorWriteEnables)
{
    unix_funcs->p_vkCmdSetColorWriteEnableEXT(commandBuffer, attachmentCount, pColorWriteEnables);
}

void WINAPI vkCmdSetCullModeEXT(VkCommandBuffer commandBuffer, VkCullModeFlags cullMode)
{
    unix_funcs->p_vkCmdSetCullModeEXT(commandBuffer, cullMode);
}

void WINAPI vkCmdSetDepthBias(VkCommandBuffer commandBuffer, float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor)
{
    unix_funcs->p_vkCmdSetDepthBias(commandBuffer, depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
}

void WINAPI vkCmdSetDepthBiasEnableEXT(VkCommandBuffer commandBuffer, VkBool32 depthBiasEnable)
{
    unix_funcs->p_vkCmdSetDepthBiasEnableEXT(commandBuffer, depthBiasEnable);
}

void WINAPI vkCmdSetDepthBounds(VkCommandBuffer commandBuffer, float minDepthBounds, float maxDepthBounds)
{
    unix_funcs->p_vkCmdSetDepthBounds(commandBuffer, minDepthBounds, maxDepthBounds);
}

void WINAPI vkCmdSetDepthBoundsTestEnableEXT(VkCommandBuffer commandBuffer, VkBool32 depthBoundsTestEnable)
{
    unix_funcs->p_vkCmdSetDepthBoundsTestEnableEXT(commandBuffer, depthBoundsTestEnable);
}

void WINAPI vkCmdSetDepthCompareOpEXT(VkCommandBuffer commandBuffer, VkCompareOp depthCompareOp)
{
    unix_funcs->p_vkCmdSetDepthCompareOpEXT(commandBuffer, depthCompareOp);
}

void WINAPI vkCmdSetDepthTestEnableEXT(VkCommandBuffer commandBuffer, VkBool32 depthTestEnable)
{
    unix_funcs->p_vkCmdSetDepthTestEnableEXT(commandBuffer, depthTestEnable);
}

void WINAPI vkCmdSetDepthWriteEnableEXT(VkCommandBuffer commandBuffer, VkBool32 depthWriteEnable)
{
    unix_funcs->p_vkCmdSetDepthWriteEnableEXT(commandBuffer, depthWriteEnable);
}

void WINAPI vkCmdSetDeviceMask(VkCommandBuffer commandBuffer, uint32_t deviceMask)
{
    unix_funcs->p_vkCmdSetDeviceMask(commandBuffer, deviceMask);
}

void WINAPI vkCmdSetDeviceMaskKHR(VkCommandBuffer commandBuffer, uint32_t deviceMask)
{
    unix_funcs->p_vkCmdSetDeviceMaskKHR(commandBuffer, deviceMask);
}

void WINAPI vkCmdSetDiscardRectangleEXT(VkCommandBuffer commandBuffer, uint32_t firstDiscardRectangle, uint32_t discardRectangleCount, const VkRect2D *pDiscardRectangles)
{
    unix_funcs->p_vkCmdSetDiscardRectangleEXT(commandBuffer, firstDiscardRectangle, discardRectangleCount, pDiscardRectangles);
}

void WINAPI vkCmdSetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask)
{
    unix_funcs->p_vkCmdSetEvent(commandBuffer, event, stageMask);
}

void WINAPI vkCmdSetEvent2KHR(VkCommandBuffer commandBuffer, VkEvent event, const VkDependencyInfoKHR *pDependencyInfo)
{
    unix_funcs->p_vkCmdSetEvent2KHR(commandBuffer, event, pDependencyInfo);
}

void WINAPI vkCmdSetExclusiveScissorNV(VkCommandBuffer commandBuffer, uint32_t firstExclusiveScissor, uint32_t exclusiveScissorCount, const VkRect2D *pExclusiveScissors)
{
    unix_funcs->p_vkCmdSetExclusiveScissorNV(commandBuffer, firstExclusiveScissor, exclusiveScissorCount, pExclusiveScissors);
}

void WINAPI vkCmdSetFragmentShadingRateEnumNV(VkCommandBuffer commandBuffer, VkFragmentShadingRateNV shadingRate, const VkFragmentShadingRateCombinerOpKHR combinerOps[2])
{
    unix_funcs->p_vkCmdSetFragmentShadingRateEnumNV(commandBuffer, shadingRate, combinerOps);
}

void WINAPI vkCmdSetFragmentShadingRateKHR(VkCommandBuffer commandBuffer, const VkExtent2D *pFragmentSize, const VkFragmentShadingRateCombinerOpKHR combinerOps[2])
{
    unix_funcs->p_vkCmdSetFragmentShadingRateKHR(commandBuffer, pFragmentSize, combinerOps);
}

void WINAPI vkCmdSetFrontFaceEXT(VkCommandBuffer commandBuffer, VkFrontFace frontFace)
{
    unix_funcs->p_vkCmdSetFrontFaceEXT(commandBuffer, frontFace);
}

void WINAPI vkCmdSetLineStippleEXT(VkCommandBuffer commandBuffer, uint32_t lineStippleFactor, uint16_t lineStipplePattern)
{
    unix_funcs->p_vkCmdSetLineStippleEXT(commandBuffer, lineStippleFactor, lineStipplePattern);
}

void WINAPI vkCmdSetLineWidth(VkCommandBuffer commandBuffer, float lineWidth)
{
    unix_funcs->p_vkCmdSetLineWidth(commandBuffer, lineWidth);
}

void WINAPI vkCmdSetLogicOpEXT(VkCommandBuffer commandBuffer, VkLogicOp logicOp)
{
    unix_funcs->p_vkCmdSetLogicOpEXT(commandBuffer, logicOp);
}

void WINAPI vkCmdSetPatchControlPointsEXT(VkCommandBuffer commandBuffer, uint32_t patchControlPoints)
{
    unix_funcs->p_vkCmdSetPatchControlPointsEXT(commandBuffer, patchControlPoints);
}

VkResult WINAPI vkCmdSetPerformanceMarkerINTEL(VkCommandBuffer commandBuffer, const VkPerformanceMarkerInfoINTEL *pMarkerInfo)
{
    return unix_funcs->p_vkCmdSetPerformanceMarkerINTEL(commandBuffer, pMarkerInfo);
}

VkResult WINAPI vkCmdSetPerformanceOverrideINTEL(VkCommandBuffer commandBuffer, const VkPerformanceOverrideInfoINTEL *pOverrideInfo)
{
    return unix_funcs->p_vkCmdSetPerformanceOverrideINTEL(commandBuffer, pOverrideInfo);
}

VkResult WINAPI vkCmdSetPerformanceStreamMarkerINTEL(VkCommandBuffer commandBuffer, const VkPerformanceStreamMarkerInfoINTEL *pMarkerInfo)
{
    return unix_funcs->p_vkCmdSetPerformanceStreamMarkerINTEL(commandBuffer, pMarkerInfo);
}

void WINAPI vkCmdSetPrimitiveRestartEnableEXT(VkCommandBuffer commandBuffer, VkBool32 primitiveRestartEnable)
{
    unix_funcs->p_vkCmdSetPrimitiveRestartEnableEXT(commandBuffer, primitiveRestartEnable);
}

void WINAPI vkCmdSetPrimitiveTopologyEXT(VkCommandBuffer commandBuffer, VkPrimitiveTopology primitiveTopology)
{
    unix_funcs->p_vkCmdSetPrimitiveTopologyEXT(commandBuffer, primitiveTopology);
}

void WINAPI vkCmdSetRasterizerDiscardEnableEXT(VkCommandBuffer commandBuffer, VkBool32 rasterizerDiscardEnable)
{
    unix_funcs->p_vkCmdSetRasterizerDiscardEnableEXT(commandBuffer, rasterizerDiscardEnable);
}

void WINAPI vkCmdSetRayTracingPipelineStackSizeKHR(VkCommandBuffer commandBuffer, uint32_t pipelineStackSize)
{
    unix_funcs->p_vkCmdSetRayTracingPipelineStackSizeKHR(commandBuffer, pipelineStackSize);
}

void WINAPI vkCmdSetSampleLocationsEXT(VkCommandBuffer commandBuffer, const VkSampleLocationsInfoEXT *pSampleLocationsInfo)
{
    unix_funcs->p_vkCmdSetSampleLocationsEXT(commandBuffer, pSampleLocationsInfo);
}

void WINAPI vkCmdSetScissor(VkCommandBuffer commandBuffer, uint32_t firstScissor, uint32_t scissorCount, const VkRect2D *pScissors)
{
    unix_funcs->p_vkCmdSetScissor(commandBuffer, firstScissor, scissorCount, pScissors);
}

void WINAPI vkCmdSetScissorWithCountEXT(VkCommandBuffer commandBuffer, uint32_t scissorCount, const VkRect2D *pScissors)
{
    unix_funcs->p_vkCmdSetScissorWithCountEXT(commandBuffer, scissorCount, pScissors);
}

void WINAPI vkCmdSetStencilCompareMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t compareMask)
{
    unix_funcs->p_vkCmdSetStencilCompareMask(commandBuffer, faceMask, compareMask);
}

void WINAPI vkCmdSetStencilOpEXT(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, VkStencilOp failOp, VkStencilOp passOp, VkStencilOp depthFailOp, VkCompareOp compareOp)
{
    unix_funcs->p_vkCmdSetStencilOpEXT(commandBuffer, faceMask, failOp, passOp, depthFailOp, compareOp);
}

void WINAPI vkCmdSetStencilReference(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t reference)
{
    unix_funcs->p_vkCmdSetStencilReference(commandBuffer, faceMask, reference);
}

void WINAPI vkCmdSetStencilTestEnableEXT(VkCommandBuffer commandBuffer, VkBool32 stencilTestEnable)
{
    unix_funcs->p_vkCmdSetStencilTestEnableEXT(commandBuffer, stencilTestEnable);
}

void WINAPI vkCmdSetStencilWriteMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t writeMask)
{
    unix_funcs->p_vkCmdSetStencilWriteMask(commandBuffer, faceMask, writeMask);
}

void WINAPI vkCmdSetVertexInputEXT(VkCommandBuffer commandBuffer, uint32_t vertexBindingDescriptionCount, const VkVertexInputBindingDescription2EXT *pVertexBindingDescriptions, uint32_t vertexAttributeDescriptionCount, const VkVertexInputAttributeDescription2EXT *pVertexAttributeDescriptions)
{
    unix_funcs->p_vkCmdSetVertexInputEXT(commandBuffer, vertexBindingDescriptionCount, pVertexBindingDescriptions, vertexAttributeDescriptionCount, pVertexAttributeDescriptions);
}

void WINAPI vkCmdSetViewport(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkViewport *pViewports)
{
    unix_funcs->p_vkCmdSetViewport(commandBuffer, firstViewport, viewportCount, pViewports);
}

void WINAPI vkCmdSetViewportShadingRatePaletteNV(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkShadingRatePaletteNV *pShadingRatePalettes)
{
    unix_funcs->p_vkCmdSetViewportShadingRatePaletteNV(commandBuffer, firstViewport, viewportCount, pShadingRatePalettes);
}

void WINAPI vkCmdSetViewportWScalingNV(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkViewportWScalingNV *pViewportWScalings)
{
    unix_funcs->p_vkCmdSetViewportWScalingNV(commandBuffer, firstViewport, viewportCount, pViewportWScalings);
}

void WINAPI vkCmdSetViewportWithCountEXT(VkCommandBuffer commandBuffer, uint32_t viewportCount, const VkViewport *pViewports)
{
    unix_funcs->p_vkCmdSetViewportWithCountEXT(commandBuffer, viewportCount, pViewports);
}

void WINAPI vkCmdTraceRaysIndirectKHR(VkCommandBuffer commandBuffer, const VkStridedDeviceAddressRegionKHR *pRaygenShaderBindingTable, const VkStridedDeviceAddressRegionKHR *pMissShaderBindingTable, const VkStridedDeviceAddressRegionKHR *pHitShaderBindingTable, const VkStridedDeviceAddressRegionKHR *pCallableShaderBindingTable, VkDeviceAddress indirectDeviceAddress)
{
    unix_funcs->p_vkCmdTraceRaysIndirectKHR(commandBuffer, pRaygenShaderBindingTable, pMissShaderBindingTable, pHitShaderBindingTable, pCallableShaderBindingTable, indirectDeviceAddress);
}

void WINAPI vkCmdTraceRaysKHR(VkCommandBuffer commandBuffer, const VkStridedDeviceAddressRegionKHR *pRaygenShaderBindingTable, const VkStridedDeviceAddressRegionKHR *pMissShaderBindingTable, const VkStridedDeviceAddressRegionKHR *pHitShaderBindingTable, const VkStridedDeviceAddressRegionKHR *pCallableShaderBindingTable, uint32_t width, uint32_t height, uint32_t depth)
{
    unix_funcs->p_vkCmdTraceRaysKHR(commandBuffer, pRaygenShaderBindingTable, pMissShaderBindingTable, pHitShaderBindingTable, pCallableShaderBindingTable, width, height, depth);
}

void WINAPI vkCmdTraceRaysNV(VkCommandBuffer commandBuffer, VkBuffer raygenShaderBindingTableBuffer, VkDeviceSize raygenShaderBindingOffset, VkBuffer missShaderBindingTableBuffer, VkDeviceSize missShaderBindingOffset, VkDeviceSize missShaderBindingStride, VkBuffer hitShaderBindingTableBuffer, VkDeviceSize hitShaderBindingOffset, VkDeviceSize hitShaderBindingStride, VkBuffer callableShaderBindingTableBuffer, VkDeviceSize callableShaderBindingOffset, VkDeviceSize callableShaderBindingStride, uint32_t width, uint32_t height, uint32_t depth)
{
    unix_funcs->p_vkCmdTraceRaysNV(commandBuffer, raygenShaderBindingTableBuffer, raygenShaderBindingOffset, missShaderBindingTableBuffer, missShaderBindingOffset, missShaderBindingStride, hitShaderBindingTableBuffer, hitShaderBindingOffset, hitShaderBindingStride, callableShaderBindingTableBuffer, callableShaderBindingOffset, callableShaderBindingStride, width, height, depth);
}

void WINAPI vkCmdUpdateBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void *pData)
{
    unix_funcs->p_vkCmdUpdateBuffer(commandBuffer, dstBuffer, dstOffset, dataSize, pData);
}

void WINAPI vkCmdWaitEvents(VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent *pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier *pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier *pImageMemoryBarriers)
{
    unix_funcs->p_vkCmdWaitEvents(commandBuffer, eventCount, pEvents, srcStageMask, dstStageMask, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
}

void WINAPI vkCmdWaitEvents2KHR(VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent *pEvents, const VkDependencyInfoKHR *pDependencyInfos)
{
    unix_funcs->p_vkCmdWaitEvents2KHR(commandBuffer, eventCount, pEvents, pDependencyInfos);
}

void WINAPI vkCmdWriteAccelerationStructuresPropertiesKHR(VkCommandBuffer commandBuffer, uint32_t accelerationStructureCount, const VkAccelerationStructureKHR *pAccelerationStructures, VkQueryType queryType, VkQueryPool queryPool, uint32_t firstQuery)
{
    unix_funcs->p_vkCmdWriteAccelerationStructuresPropertiesKHR(commandBuffer, accelerationStructureCount, pAccelerationStructures, queryType, queryPool, firstQuery);
}

void WINAPI vkCmdWriteAccelerationStructuresPropertiesNV(VkCommandBuffer commandBuffer, uint32_t accelerationStructureCount, const VkAccelerationStructureNV *pAccelerationStructures, VkQueryType queryType, VkQueryPool queryPool, uint32_t firstQuery)
{
    unix_funcs->p_vkCmdWriteAccelerationStructuresPropertiesNV(commandBuffer, accelerationStructureCount, pAccelerationStructures, queryType, queryPool, firstQuery);
}

void WINAPI vkCmdWriteBufferMarker2AMD(VkCommandBuffer commandBuffer, VkPipelineStageFlags2KHR stage, VkBuffer dstBuffer, VkDeviceSize dstOffset, uint32_t marker)
{
    unix_funcs->p_vkCmdWriteBufferMarker2AMD(commandBuffer, stage, dstBuffer, dstOffset, marker);
}

void WINAPI vkCmdWriteBufferMarkerAMD(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, VkBuffer dstBuffer, VkDeviceSize dstOffset, uint32_t marker)
{
    unix_funcs->p_vkCmdWriteBufferMarkerAMD(commandBuffer, pipelineStage, dstBuffer, dstOffset, marker);
}

void WINAPI vkCmdWriteTimestamp(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool, uint32_t query)
{
    unix_funcs->p_vkCmdWriteTimestamp(commandBuffer, pipelineStage, queryPool, query);
}

void WINAPI vkCmdWriteTimestamp2KHR(VkCommandBuffer commandBuffer, VkPipelineStageFlags2KHR stage, VkQueryPool queryPool, uint32_t query)
{
    unix_funcs->p_vkCmdWriteTimestamp2KHR(commandBuffer, stage, queryPool, query);
}

VkResult WINAPI vkCompileDeferredNV(VkDevice device, VkPipeline pipeline, uint32_t shader)
{
    return unix_funcs->p_vkCompileDeferredNV(device, pipeline, shader);
}

VkResult WINAPI vkCopyAccelerationStructureKHR(VkDevice device, VkDeferredOperationKHR deferredOperation, const VkCopyAccelerationStructureInfoKHR *pInfo)
{
    return unix_funcs->p_vkCopyAccelerationStructureKHR(device, deferredOperation, pInfo);
}

VkResult WINAPI vkCopyAccelerationStructureToMemoryKHR(VkDevice device, VkDeferredOperationKHR deferredOperation, const VkCopyAccelerationStructureToMemoryInfoKHR *pInfo)
{
    return unix_funcs->p_vkCopyAccelerationStructureToMemoryKHR(device, deferredOperation, pInfo);
}

VkResult WINAPI vkCopyMemoryToAccelerationStructureKHR(VkDevice device, VkDeferredOperationKHR deferredOperation, const VkCopyMemoryToAccelerationStructureInfoKHR *pInfo)
{
    return unix_funcs->p_vkCopyMemoryToAccelerationStructureKHR(device, deferredOperation, pInfo);
}

VkResult WINAPI vkCreateAccelerationStructureKHR(VkDevice device, const VkAccelerationStructureCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkAccelerationStructureKHR *pAccelerationStructure)
{
    return unix_funcs->p_vkCreateAccelerationStructureKHR(device, pCreateInfo, pAllocator, pAccelerationStructure);
}

VkResult WINAPI vkCreateAccelerationStructureNV(VkDevice device, const VkAccelerationStructureCreateInfoNV *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkAccelerationStructureNV *pAccelerationStructure)
{
    return unix_funcs->p_vkCreateAccelerationStructureNV(device, pCreateInfo, pAllocator, pAccelerationStructure);
}

VkResult WINAPI vkCreateBuffer(VkDevice device, const VkBufferCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkBuffer *pBuffer)
{
    return unix_funcs->p_vkCreateBuffer(device, pCreateInfo, pAllocator, pBuffer);
}

VkResult WINAPI vkCreateBufferView(VkDevice device, const VkBufferViewCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkBufferView *pView)
{
    return unix_funcs->p_vkCreateBufferView(device, pCreateInfo, pAllocator, pView);
}

VkResult WINAPI vkCreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkCommandPool *pCommandPool)
{
    return unix_funcs->p_vkCreateCommandPool(device, pCreateInfo, pAllocator, pCommandPool);
}

VkResult WINAPI vkCreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkComputePipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines)
{
    return unix_funcs->p_vkCreateComputePipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
}

VkResult WINAPI vkCreateCuFunctionNVX(VkDevice device, const VkCuFunctionCreateInfoNVX *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkCuFunctionNVX *pFunction)
{
    return unix_funcs->p_vkCreateCuFunctionNVX(device, pCreateInfo, pAllocator, pFunction);
}

VkResult WINAPI vkCreateCuModuleNVX(VkDevice device, const VkCuModuleCreateInfoNVX *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkCuModuleNVX *pModule)
{
    return unix_funcs->p_vkCreateCuModuleNVX(device, pCreateInfo, pAllocator, pModule);
}

VkResult WINAPI vkCreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugReportCallbackEXT *pCallback)
{
    return unix_funcs->p_vkCreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pCallback);
}

VkResult WINAPI vkCreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pMessenger)
{
    return unix_funcs->p_vkCreateDebugUtilsMessengerEXT(instance, pCreateInfo, pAllocator, pMessenger);
}

VkResult WINAPI vkCreateDeferredOperationKHR(VkDevice device, const VkAllocationCallbacks *pAllocator, VkDeferredOperationKHR *pDeferredOperation)
{
    return unix_funcs->p_vkCreateDeferredOperationKHR(device, pAllocator, pDeferredOperation);
}

VkResult WINAPI vkCreateDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDescriptorPool *pDescriptorPool)
{
    return unix_funcs->p_vkCreateDescriptorPool(device, pCreateInfo, pAllocator, pDescriptorPool);
}

VkResult WINAPI vkCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDescriptorSetLayout *pSetLayout)
{
    return unix_funcs->p_vkCreateDescriptorSetLayout(device, pCreateInfo, pAllocator, pSetLayout);
}

VkResult WINAPI vkCreateDescriptorUpdateTemplate(VkDevice device, const VkDescriptorUpdateTemplateCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDescriptorUpdateTemplate *pDescriptorUpdateTemplate)
{
    return unix_funcs->p_vkCreateDescriptorUpdateTemplate(device, pCreateInfo, pAllocator, pDescriptorUpdateTemplate);
}

VkResult WINAPI vkCreateDescriptorUpdateTemplateKHR(VkDevice device, const VkDescriptorUpdateTemplateCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDescriptorUpdateTemplate *pDescriptorUpdateTemplate)
{
    return unix_funcs->p_vkCreateDescriptorUpdateTemplateKHR(device, pCreateInfo, pAllocator, pDescriptorUpdateTemplate);
}

VkResult WINAPI vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDevice *pDevice)
{
    return unix_funcs->p_vkCreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);
}

VkResult WINAPI vkCreateEvent(VkDevice device, const VkEventCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkEvent *pEvent)
{
    return unix_funcs->p_vkCreateEvent(device, pCreateInfo, pAllocator, pEvent);
}

VkResult WINAPI vkCreateFence(VkDevice device, const VkFenceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkFence *pFence)
{
    return unix_funcs->p_vkCreateFence(device, pCreateInfo, pAllocator, pFence);
}

VkResult WINAPI vkCreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkFramebuffer *pFramebuffer)
{
    return unix_funcs->p_vkCreateFramebuffer(device, pCreateInfo, pAllocator, pFramebuffer);
}

VkResult WINAPI vkCreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines)
{
    return unix_funcs->p_vkCreateGraphicsPipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
}

VkResult WINAPI vkCreateImage(VkDevice device, const VkImageCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkImage *pImage)
{
    return unix_funcs->p_vkCreateImage(device, pCreateInfo, pAllocator, pImage);
}

VkResult WINAPI vkCreateImageView(VkDevice device, const VkImageViewCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkImageView *pView)
{
    return unix_funcs->p_vkCreateImageView(device, pCreateInfo, pAllocator, pView);
}

VkResult WINAPI vkCreateIndirectCommandsLayoutNV(VkDevice device, const VkIndirectCommandsLayoutCreateInfoNV *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkIndirectCommandsLayoutNV *pIndirectCommandsLayout)
{
    return unix_funcs->p_vkCreateIndirectCommandsLayoutNV(device, pCreateInfo, pAllocator, pIndirectCommandsLayout);
}

VkResult WINAPI vkCreatePipelineCache(VkDevice device, const VkPipelineCacheCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkPipelineCache *pPipelineCache)
{
    return unix_funcs->p_vkCreatePipelineCache(device, pCreateInfo, pAllocator, pPipelineCache);
}

VkResult WINAPI vkCreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkPipelineLayout *pPipelineLayout)
{
    return unix_funcs->p_vkCreatePipelineLayout(device, pCreateInfo, pAllocator, pPipelineLayout);
}

VkResult WINAPI vkCreatePrivateDataSlotEXT(VkDevice device, const VkPrivateDataSlotCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkPrivateDataSlotEXT *pPrivateDataSlot)
{
    return unix_funcs->p_vkCreatePrivateDataSlotEXT(device, pCreateInfo, pAllocator, pPrivateDataSlot);
}

VkResult WINAPI vkCreateQueryPool(VkDevice device, const VkQueryPoolCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkQueryPool *pQueryPool)
{
    return unix_funcs->p_vkCreateQueryPool(device, pCreateInfo, pAllocator, pQueryPool);
}

VkResult WINAPI vkCreateRayTracingPipelinesKHR(VkDevice device, VkDeferredOperationKHR deferredOperation, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkRayTracingPipelineCreateInfoKHR *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines)
{
    return unix_funcs->p_vkCreateRayTracingPipelinesKHR(device, deferredOperation, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
}

VkResult WINAPI vkCreateRayTracingPipelinesNV(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkRayTracingPipelineCreateInfoNV *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines)
{
    return unix_funcs->p_vkCreateRayTracingPipelinesNV(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
}

VkResult WINAPI vkCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkRenderPass *pRenderPass)
{
    return unix_funcs->p_vkCreateRenderPass(device, pCreateInfo, pAllocator, pRenderPass);
}

VkResult WINAPI vkCreateRenderPass2(VkDevice device, const VkRenderPassCreateInfo2 *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkRenderPass *pRenderPass)
{
    return unix_funcs->p_vkCreateRenderPass2(device, pCreateInfo, pAllocator, pRenderPass);
}

VkResult WINAPI vkCreateRenderPass2KHR(VkDevice device, const VkRenderPassCreateInfo2 *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkRenderPass *pRenderPass)
{
    return unix_funcs->p_vkCreateRenderPass2KHR(device, pCreateInfo, pAllocator, pRenderPass);
}

VkResult WINAPI vkCreateSampler(VkDevice device, const VkSamplerCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSampler *pSampler)
{
    return unix_funcs->p_vkCreateSampler(device, pCreateInfo, pAllocator, pSampler);
}

VkResult WINAPI vkCreateSamplerYcbcrConversion(VkDevice device, const VkSamplerYcbcrConversionCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSamplerYcbcrConversion *pYcbcrConversion)
{
    return unix_funcs->p_vkCreateSamplerYcbcrConversion(device, pCreateInfo, pAllocator, pYcbcrConversion);
}

VkResult WINAPI vkCreateSamplerYcbcrConversionKHR(VkDevice device, const VkSamplerYcbcrConversionCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSamplerYcbcrConversion *pYcbcrConversion)
{
    return unix_funcs->p_vkCreateSamplerYcbcrConversionKHR(device, pCreateInfo, pAllocator, pYcbcrConversion);
}

VkResult WINAPI vkCreateSemaphore(VkDevice device, const VkSemaphoreCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSemaphore *pSemaphore)
{
    return unix_funcs->p_vkCreateSemaphore(device, pCreateInfo, pAllocator, pSemaphore);
}

VkResult WINAPI vkCreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkShaderModule *pShaderModule)
{
    return unix_funcs->p_vkCreateShaderModule(device, pCreateInfo, pAllocator, pShaderModule);
}

VkResult WINAPI vkCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSwapchainKHR *pSwapchain)
{
    return unix_funcs->p_vkCreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);
}

VkResult WINAPI vkCreateValidationCacheEXT(VkDevice device, const VkValidationCacheCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkValidationCacheEXT *pValidationCache)
{
    return unix_funcs->p_vkCreateValidationCacheEXT(device, pCreateInfo, pAllocator, pValidationCache);
}

VkResult WINAPI vkCreateWin32SurfaceKHR(VkInstance instance, const VkWin32SurfaceCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface)
{
    return unix_funcs->p_vkCreateWin32SurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
}

VkResult WINAPI vkDebugMarkerSetObjectNameEXT(VkDevice device, const VkDebugMarkerObjectNameInfoEXT *pNameInfo)
{
    return unix_funcs->p_vkDebugMarkerSetObjectNameEXT(device, pNameInfo);
}

VkResult WINAPI vkDebugMarkerSetObjectTagEXT(VkDevice device, const VkDebugMarkerObjectTagInfoEXT *pTagInfo)
{
    return unix_funcs->p_vkDebugMarkerSetObjectTagEXT(device, pTagInfo);
}

void WINAPI vkDebugReportMessageEXT(VkInstance instance, VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char *pLayerPrefix, const char *pMessage)
{
    unix_funcs->p_vkDebugReportMessageEXT(instance, flags, objectType, object, location, messageCode, pLayerPrefix, pMessage);
}

VkResult WINAPI vkDeferredOperationJoinKHR(VkDevice device, VkDeferredOperationKHR operation)
{
    return unix_funcs->p_vkDeferredOperationJoinKHR(device, operation);
}

void WINAPI vkDestroyAccelerationStructureKHR(VkDevice device, VkAccelerationStructureKHR accelerationStructure, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyAccelerationStructureKHR(device, accelerationStructure, pAllocator);
}

void WINAPI vkDestroyAccelerationStructureNV(VkDevice device, VkAccelerationStructureNV accelerationStructure, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyAccelerationStructureNV(device, accelerationStructure, pAllocator);
}

void WINAPI vkDestroyBuffer(VkDevice device, VkBuffer buffer, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyBuffer(device, buffer, pAllocator);
}

void WINAPI vkDestroyBufferView(VkDevice device, VkBufferView bufferView, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyBufferView(device, bufferView, pAllocator);
}

void WINAPI vkDestroyCommandPool(VkDevice device, VkCommandPool commandPool, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyCommandPool(device, commandPool, pAllocator);
}

void WINAPI vkDestroyCuFunctionNVX(VkDevice device, VkCuFunctionNVX function, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyCuFunctionNVX(device, function, pAllocator);
}

void WINAPI vkDestroyCuModuleNVX(VkDevice device, VkCuModuleNVX module, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyCuModuleNVX(device, module, pAllocator);
}

void WINAPI vkDestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyDebugReportCallbackEXT(instance, callback, pAllocator);
}

void WINAPI vkDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyDebugUtilsMessengerEXT(instance, messenger, pAllocator);
}

void WINAPI vkDestroyDeferredOperationKHR(VkDevice device, VkDeferredOperationKHR operation, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyDeferredOperationKHR(device, operation, pAllocator);
}

void WINAPI vkDestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyDescriptorPool(device, descriptorPool, pAllocator);
}

void WINAPI vkDestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyDescriptorSetLayout(device, descriptorSetLayout, pAllocator);
}

void WINAPI vkDestroyDescriptorUpdateTemplate(VkDevice device, VkDescriptorUpdateTemplate descriptorUpdateTemplate, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyDescriptorUpdateTemplate(device, descriptorUpdateTemplate, pAllocator);
}

void WINAPI vkDestroyDescriptorUpdateTemplateKHR(VkDevice device, VkDescriptorUpdateTemplate descriptorUpdateTemplate, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyDescriptorUpdateTemplateKHR(device, descriptorUpdateTemplate, pAllocator);
}

void WINAPI vkDestroyDevice(VkDevice device, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyDevice(device, pAllocator);
}

void WINAPI vkDestroyEvent(VkDevice device, VkEvent event, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyEvent(device, event, pAllocator);
}

void WINAPI vkDestroyFence(VkDevice device, VkFence fence, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyFence(device, fence, pAllocator);
}

void WINAPI vkDestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyFramebuffer(device, framebuffer, pAllocator);
}

void WINAPI vkDestroyImage(VkDevice device, VkImage image, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyImage(device, image, pAllocator);
}

void WINAPI vkDestroyImageView(VkDevice device, VkImageView imageView, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyImageView(device, imageView, pAllocator);
}

void WINAPI vkDestroyIndirectCommandsLayoutNV(VkDevice device, VkIndirectCommandsLayoutNV indirectCommandsLayout, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyIndirectCommandsLayoutNV(device, indirectCommandsLayout, pAllocator);
}

void WINAPI vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyInstance(instance, pAllocator);
}

void WINAPI vkDestroyPipeline(VkDevice device, VkPipeline pipeline, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyPipeline(device, pipeline, pAllocator);
}

void WINAPI vkDestroyPipelineCache(VkDevice device, VkPipelineCache pipelineCache, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyPipelineCache(device, pipelineCache, pAllocator);
}

void WINAPI vkDestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyPipelineLayout(device, pipelineLayout, pAllocator);
}

void WINAPI vkDestroyPrivateDataSlotEXT(VkDevice device, VkPrivateDataSlotEXT privateDataSlot, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyPrivateDataSlotEXT(device, privateDataSlot, pAllocator);
}

void WINAPI vkDestroyQueryPool(VkDevice device, VkQueryPool queryPool, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyQueryPool(device, queryPool, pAllocator);
}

void WINAPI vkDestroyRenderPass(VkDevice device, VkRenderPass renderPass, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyRenderPass(device, renderPass, pAllocator);
}

void WINAPI vkDestroySampler(VkDevice device, VkSampler sampler, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroySampler(device, sampler, pAllocator);
}

void WINAPI vkDestroySamplerYcbcrConversion(VkDevice device, VkSamplerYcbcrConversion ycbcrConversion, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroySamplerYcbcrConversion(device, ycbcrConversion, pAllocator);
}

void WINAPI vkDestroySamplerYcbcrConversionKHR(VkDevice device, VkSamplerYcbcrConversion ycbcrConversion, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroySamplerYcbcrConversionKHR(device, ycbcrConversion, pAllocator);
}

void WINAPI vkDestroySemaphore(VkDevice device, VkSemaphore semaphore, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroySemaphore(device, semaphore, pAllocator);
}

void WINAPI vkDestroyShaderModule(VkDevice device, VkShaderModule shaderModule, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyShaderModule(device, shaderModule, pAllocator);
}

void WINAPI vkDestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroySurfaceKHR(instance, surface, pAllocator);
}

void WINAPI vkDestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroySwapchainKHR(device, swapchain, pAllocator);
}

void WINAPI vkDestroyValidationCacheEXT(VkDevice device, VkValidationCacheEXT validationCache, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkDestroyValidationCacheEXT(device, validationCache, pAllocator);
}

VkResult WINAPI vkDeviceWaitIdle(VkDevice device)
{
    return unix_funcs->p_vkDeviceWaitIdle(device);
}

VkResult WINAPI vkEndCommandBuffer(VkCommandBuffer commandBuffer)
{
    return unix_funcs->p_vkEndCommandBuffer(commandBuffer);
}

VkResult WINAPI vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char *pLayerName, uint32_t *pPropertyCount, VkExtensionProperties *pProperties)
{
    return unix_funcs->p_vkEnumerateDeviceExtensionProperties(physicalDevice, pLayerName, pPropertyCount, pProperties);
}

VkResult WINAPI vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount, VkLayerProperties *pProperties)
{
    return unix_funcs->p_vkEnumerateDeviceLayerProperties(physicalDevice, pPropertyCount, pProperties);
}

VkResult WINAPI vkEnumeratePhysicalDeviceGroups(VkInstance instance, uint32_t *pPhysicalDeviceGroupCount, VkPhysicalDeviceGroupProperties *pPhysicalDeviceGroupProperties)
{
    return unix_funcs->p_vkEnumeratePhysicalDeviceGroups(instance, pPhysicalDeviceGroupCount, pPhysicalDeviceGroupProperties);
}

VkResult WINAPI vkEnumeratePhysicalDeviceGroupsKHR(VkInstance instance, uint32_t *pPhysicalDeviceGroupCount, VkPhysicalDeviceGroupProperties *pPhysicalDeviceGroupProperties)
{
    return unix_funcs->p_vkEnumeratePhysicalDeviceGroupsKHR(instance, pPhysicalDeviceGroupCount, pPhysicalDeviceGroupProperties);
}

VkResult WINAPI vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, uint32_t *pCounterCount, VkPerformanceCounterKHR *pCounters, VkPerformanceCounterDescriptionKHR *pCounterDescriptions)
{
    return unix_funcs->p_vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR(physicalDevice, queueFamilyIndex, pCounterCount, pCounters, pCounterDescriptions);
}

VkResult WINAPI vkEnumeratePhysicalDevices(VkInstance instance, uint32_t *pPhysicalDeviceCount, VkPhysicalDevice *pPhysicalDevices)
{
    return unix_funcs->p_vkEnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices);
}

VkResult WINAPI vkFlushMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount, const VkMappedMemoryRange *pMemoryRanges)
{
    return unix_funcs->p_vkFlushMappedMemoryRanges(device, memoryRangeCount, pMemoryRanges);
}

void WINAPI vkFreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount, const VkCommandBuffer *pCommandBuffers)
{
    unix_funcs->p_vkFreeCommandBuffers(device, commandPool, commandBufferCount, pCommandBuffers);
}

VkResult WINAPI vkFreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t descriptorSetCount, const VkDescriptorSet *pDescriptorSets)
{
    return unix_funcs->p_vkFreeDescriptorSets(device, descriptorPool, descriptorSetCount, pDescriptorSets);
}

void WINAPI vkFreeMemory(VkDevice device, VkDeviceMemory memory, const VkAllocationCallbacks *pAllocator)
{
    unix_funcs->p_vkFreeMemory(device, memory, pAllocator);
}

void WINAPI vkGetAccelerationStructureBuildSizesKHR(VkDevice device, VkAccelerationStructureBuildTypeKHR buildType, const VkAccelerationStructureBuildGeometryInfoKHR *pBuildInfo, const uint32_t *pMaxPrimitiveCounts, VkAccelerationStructureBuildSizesInfoKHR *pSizeInfo)
{
    unix_funcs->p_vkGetAccelerationStructureBuildSizesKHR(device, buildType, pBuildInfo, pMaxPrimitiveCounts, pSizeInfo);
}

VkDeviceAddress WINAPI vkGetAccelerationStructureDeviceAddressKHR(VkDevice device, const VkAccelerationStructureDeviceAddressInfoKHR *pInfo)
{
    return unix_funcs->p_vkGetAccelerationStructureDeviceAddressKHR(device, pInfo);
}

VkResult WINAPI vkGetAccelerationStructureHandleNV(VkDevice device, VkAccelerationStructureNV accelerationStructure, size_t dataSize, void *pData)
{
    return unix_funcs->p_vkGetAccelerationStructureHandleNV(device, accelerationStructure, dataSize, pData);
}

void WINAPI vkGetAccelerationStructureMemoryRequirementsNV(VkDevice device, const VkAccelerationStructureMemoryRequirementsInfoNV *pInfo, VkMemoryRequirements2KHR *pMemoryRequirements)
{
    unix_funcs->p_vkGetAccelerationStructureMemoryRequirementsNV(device, pInfo, pMemoryRequirements);
}

VkDeviceAddress WINAPI vkGetBufferDeviceAddress(VkDevice device, const VkBufferDeviceAddressInfo *pInfo)
{
    return unix_funcs->p_vkGetBufferDeviceAddress(device, pInfo);
}

VkDeviceAddress WINAPI vkGetBufferDeviceAddressEXT(VkDevice device, const VkBufferDeviceAddressInfo *pInfo)
{
    return unix_funcs->p_vkGetBufferDeviceAddressEXT(device, pInfo);
}

VkDeviceAddress WINAPI vkGetBufferDeviceAddressKHR(VkDevice device, const VkBufferDeviceAddressInfo *pInfo)
{
    return unix_funcs->p_vkGetBufferDeviceAddressKHR(device, pInfo);
}

void WINAPI vkGetBufferMemoryRequirements(VkDevice device, VkBuffer buffer, VkMemoryRequirements *pMemoryRequirements)
{
    unix_funcs->p_vkGetBufferMemoryRequirements(device, buffer, pMemoryRequirements);
}

void WINAPI vkGetBufferMemoryRequirements2(VkDevice device, const VkBufferMemoryRequirementsInfo2 *pInfo, VkMemoryRequirements2 *pMemoryRequirements)
{
    unix_funcs->p_vkGetBufferMemoryRequirements2(device, pInfo, pMemoryRequirements);
}

void WINAPI vkGetBufferMemoryRequirements2KHR(VkDevice device, const VkBufferMemoryRequirementsInfo2 *pInfo, VkMemoryRequirements2 *pMemoryRequirements)
{
    unix_funcs->p_vkGetBufferMemoryRequirements2KHR(device, pInfo, pMemoryRequirements);
}

uint64_t WINAPI vkGetBufferOpaqueCaptureAddress(VkDevice device, const VkBufferDeviceAddressInfo *pInfo)
{
    return unix_funcs->p_vkGetBufferOpaqueCaptureAddress(device, pInfo);
}

uint64_t WINAPI vkGetBufferOpaqueCaptureAddressKHR(VkDevice device, const VkBufferDeviceAddressInfo *pInfo)
{
    return unix_funcs->p_vkGetBufferOpaqueCaptureAddressKHR(device, pInfo);
}

VkResult WINAPI vkGetCalibratedTimestampsEXT(VkDevice device, uint32_t timestampCount, const VkCalibratedTimestampInfoEXT *pTimestampInfos, uint64_t *pTimestamps, uint64_t *pMaxDeviation)
{
    return unix_funcs->p_vkGetCalibratedTimestampsEXT(device, timestampCount, pTimestampInfos, pTimestamps, pMaxDeviation);
}

uint32_t WINAPI vkGetDeferredOperationMaxConcurrencyKHR(VkDevice device, VkDeferredOperationKHR operation)
{
    return unix_funcs->p_vkGetDeferredOperationMaxConcurrencyKHR(device, operation);
}

VkResult WINAPI vkGetDeferredOperationResultKHR(VkDevice device, VkDeferredOperationKHR operation)
{
    return unix_funcs->p_vkGetDeferredOperationResultKHR(device, operation);
}

void WINAPI vkGetDescriptorSetLayoutSupport(VkDevice device, const VkDescriptorSetLayoutCreateInfo *pCreateInfo, VkDescriptorSetLayoutSupport *pSupport)
{
    unix_funcs->p_vkGetDescriptorSetLayoutSupport(device, pCreateInfo, pSupport);
}

void WINAPI vkGetDescriptorSetLayoutSupportKHR(VkDevice device, const VkDescriptorSetLayoutCreateInfo *pCreateInfo, VkDescriptorSetLayoutSupport *pSupport)
{
    unix_funcs->p_vkGetDescriptorSetLayoutSupportKHR(device, pCreateInfo, pSupport);
}

void WINAPI vkGetDeviceAccelerationStructureCompatibilityKHR(VkDevice device, const VkAccelerationStructureVersionInfoKHR *pVersionInfo, VkAccelerationStructureCompatibilityKHR *pCompatibility)
{
    unix_funcs->p_vkGetDeviceAccelerationStructureCompatibilityKHR(device, pVersionInfo, pCompatibility);
}

void WINAPI vkGetDeviceGroupPeerMemoryFeatures(VkDevice device, uint32_t heapIndex, uint32_t localDeviceIndex, uint32_t remoteDeviceIndex, VkPeerMemoryFeatureFlags *pPeerMemoryFeatures)
{
    unix_funcs->p_vkGetDeviceGroupPeerMemoryFeatures(device, heapIndex, localDeviceIndex, remoteDeviceIndex, pPeerMemoryFeatures);
}

void WINAPI vkGetDeviceGroupPeerMemoryFeaturesKHR(VkDevice device, uint32_t heapIndex, uint32_t localDeviceIndex, uint32_t remoteDeviceIndex, VkPeerMemoryFeatureFlags *pPeerMemoryFeatures)
{
    unix_funcs->p_vkGetDeviceGroupPeerMemoryFeaturesKHR(device, heapIndex, localDeviceIndex, remoteDeviceIndex, pPeerMemoryFeatures);
}

VkResult WINAPI vkGetDeviceGroupPresentCapabilitiesKHR(VkDevice device, VkDeviceGroupPresentCapabilitiesKHR *pDeviceGroupPresentCapabilities)
{
    return unix_funcs->p_vkGetDeviceGroupPresentCapabilitiesKHR(device, pDeviceGroupPresentCapabilities);
}

VkResult WINAPI vkGetDeviceGroupSurfacePresentModesKHR(VkDevice device, VkSurfaceKHR surface, VkDeviceGroupPresentModeFlagsKHR *pModes)
{
    return unix_funcs->p_vkGetDeviceGroupSurfacePresentModesKHR(device, surface, pModes);
}

void WINAPI vkGetDeviceMemoryCommitment(VkDevice device, VkDeviceMemory memory, VkDeviceSize *pCommittedMemoryInBytes)
{
    unix_funcs->p_vkGetDeviceMemoryCommitment(device, memory, pCommittedMemoryInBytes);
}

uint64_t WINAPI vkGetDeviceMemoryOpaqueCaptureAddress(VkDevice device, const VkDeviceMemoryOpaqueCaptureAddressInfo *pInfo)
{
    return unix_funcs->p_vkGetDeviceMemoryOpaqueCaptureAddress(device, pInfo);
}

uint64_t WINAPI vkGetDeviceMemoryOpaqueCaptureAddressKHR(VkDevice device, const VkDeviceMemoryOpaqueCaptureAddressInfo *pInfo)
{
    return unix_funcs->p_vkGetDeviceMemoryOpaqueCaptureAddressKHR(device, pInfo);
}

void WINAPI vkGetDeviceQueue(VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue *pQueue)
{
    unix_funcs->p_vkGetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);
}

void WINAPI vkGetDeviceQueue2(VkDevice device, const VkDeviceQueueInfo2 *pQueueInfo, VkQueue *pQueue)
{
    unix_funcs->p_vkGetDeviceQueue2(device, pQueueInfo, pQueue);
}

VkResult WINAPI vkGetEventStatus(VkDevice device, VkEvent event)
{
    return unix_funcs->p_vkGetEventStatus(device, event);
}

VkResult WINAPI vkGetFenceStatus(VkDevice device, VkFence fence)
{
    return unix_funcs->p_vkGetFenceStatus(device, fence);
}

void WINAPI vkGetGeneratedCommandsMemoryRequirementsNV(VkDevice device, const VkGeneratedCommandsMemoryRequirementsInfoNV *pInfo, VkMemoryRequirements2 *pMemoryRequirements)
{
    unix_funcs->p_vkGetGeneratedCommandsMemoryRequirementsNV(device, pInfo, pMemoryRequirements);
}

void WINAPI vkGetImageMemoryRequirements(VkDevice device, VkImage image, VkMemoryRequirements *pMemoryRequirements)
{
    unix_funcs->p_vkGetImageMemoryRequirements(device, image, pMemoryRequirements);
}

void WINAPI vkGetImageMemoryRequirements2(VkDevice device, const VkImageMemoryRequirementsInfo2 *pInfo, VkMemoryRequirements2 *pMemoryRequirements)
{
    unix_funcs->p_vkGetImageMemoryRequirements2(device, pInfo, pMemoryRequirements);
}

void WINAPI vkGetImageMemoryRequirements2KHR(VkDevice device, const VkImageMemoryRequirementsInfo2 *pInfo, VkMemoryRequirements2 *pMemoryRequirements)
{
    unix_funcs->p_vkGetImageMemoryRequirements2KHR(device, pInfo, pMemoryRequirements);
}

void WINAPI vkGetImageSparseMemoryRequirements(VkDevice device, VkImage image, uint32_t *pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements *pSparseMemoryRequirements)
{
    unix_funcs->p_vkGetImageSparseMemoryRequirements(device, image, pSparseMemoryRequirementCount, pSparseMemoryRequirements);
}

void WINAPI vkGetImageSparseMemoryRequirements2(VkDevice device, const VkImageSparseMemoryRequirementsInfo2 *pInfo, uint32_t *pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements2 *pSparseMemoryRequirements)
{
    unix_funcs->p_vkGetImageSparseMemoryRequirements2(device, pInfo, pSparseMemoryRequirementCount, pSparseMemoryRequirements);
}

void WINAPI vkGetImageSparseMemoryRequirements2KHR(VkDevice device, const VkImageSparseMemoryRequirementsInfo2 *pInfo, uint32_t *pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements2 *pSparseMemoryRequirements)
{
    unix_funcs->p_vkGetImageSparseMemoryRequirements2KHR(device, pInfo, pSparseMemoryRequirementCount, pSparseMemoryRequirements);
}

void WINAPI vkGetImageSubresourceLayout(VkDevice device, VkImage image, const VkImageSubresource *pSubresource, VkSubresourceLayout *pLayout)
{
    unix_funcs->p_vkGetImageSubresourceLayout(device, image, pSubresource, pLayout);
}

VkResult WINAPI vkGetImageViewAddressNVX(VkDevice device, VkImageView imageView, VkImageViewAddressPropertiesNVX *pProperties)
{
    return unix_funcs->p_vkGetImageViewAddressNVX(device, imageView, pProperties);
}

uint32_t WINAPI vkGetImageViewHandleNVX(VkDevice device, const VkImageViewHandleInfoNVX *pInfo)
{
    return unix_funcs->p_vkGetImageViewHandleNVX(device, pInfo);
}

VkResult WINAPI vkGetMemoryHostPointerPropertiesEXT(VkDevice device, VkExternalMemoryHandleTypeFlagBits handleType, const void *pHostPointer, VkMemoryHostPointerPropertiesEXT *pMemoryHostPointerProperties)
{
    return unix_funcs->p_vkGetMemoryHostPointerPropertiesEXT(device, handleType, pHostPointer, pMemoryHostPointerProperties);
}

VkResult WINAPI vkGetPerformanceParameterINTEL(VkDevice device, VkPerformanceParameterTypeINTEL parameter, VkPerformanceValueINTEL *pValue)
{
    return unix_funcs->p_vkGetPerformanceParameterINTEL(device, parameter, pValue);
}

VkResult WINAPI vkGetPhysicalDeviceCalibrateableTimeDomainsEXT(VkPhysicalDevice physicalDevice, uint32_t *pTimeDomainCount, VkTimeDomainEXT *pTimeDomains)
{
    return unix_funcs->p_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT(physicalDevice, pTimeDomainCount, pTimeDomains);
}

VkResult WINAPI vkGetPhysicalDeviceCooperativeMatrixPropertiesNV(VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount, VkCooperativeMatrixPropertiesNV *pProperties)
{
    return unix_funcs->p_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV(physicalDevice, pPropertyCount, pProperties);
}

void WINAPI vkGetPhysicalDeviceExternalBufferProperties(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalBufferInfo *pExternalBufferInfo, VkExternalBufferProperties *pExternalBufferProperties)
{
    unix_funcs->p_vkGetPhysicalDeviceExternalBufferProperties(physicalDevice, pExternalBufferInfo, pExternalBufferProperties);
}

void WINAPI vkGetPhysicalDeviceExternalBufferPropertiesKHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalBufferInfo *pExternalBufferInfo, VkExternalBufferProperties *pExternalBufferProperties)
{
    unix_funcs->p_vkGetPhysicalDeviceExternalBufferPropertiesKHR(physicalDevice, pExternalBufferInfo, pExternalBufferProperties);
}

void WINAPI vkGetPhysicalDeviceExternalFenceProperties(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalFenceInfo *pExternalFenceInfo, VkExternalFenceProperties *pExternalFenceProperties)
{
    unix_funcs->p_vkGetPhysicalDeviceExternalFenceProperties(physicalDevice, pExternalFenceInfo, pExternalFenceProperties);
}

void WINAPI vkGetPhysicalDeviceExternalFencePropertiesKHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalFenceInfo *pExternalFenceInfo, VkExternalFenceProperties *pExternalFenceProperties)
{
    unix_funcs->p_vkGetPhysicalDeviceExternalFencePropertiesKHR(physicalDevice, pExternalFenceInfo, pExternalFenceProperties);
}

void WINAPI vkGetPhysicalDeviceExternalSemaphoreProperties(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalSemaphoreInfo *pExternalSemaphoreInfo, VkExternalSemaphoreProperties *pExternalSemaphoreProperties)
{
    unix_funcs->p_vkGetPhysicalDeviceExternalSemaphoreProperties(physicalDevice, pExternalSemaphoreInfo, pExternalSemaphoreProperties);
}

void WINAPI vkGetPhysicalDeviceExternalSemaphorePropertiesKHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalSemaphoreInfo *pExternalSemaphoreInfo, VkExternalSemaphoreProperties *pExternalSemaphoreProperties)
{
    unix_funcs->p_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR(physicalDevice, pExternalSemaphoreInfo, pExternalSemaphoreProperties);
}

void WINAPI vkGetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures *pFeatures)
{
    unix_funcs->p_vkGetPhysicalDeviceFeatures(physicalDevice, pFeatures);
}

void WINAPI vkGetPhysicalDeviceFeatures2(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2 *pFeatures)
{
    unix_funcs->p_vkGetPhysicalDeviceFeatures2(physicalDevice, pFeatures);
}

void WINAPI vkGetPhysicalDeviceFeatures2KHR(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2 *pFeatures)
{
    unix_funcs->p_vkGetPhysicalDeviceFeatures2KHR(physicalDevice, pFeatures);
}

void WINAPI vkGetPhysicalDeviceFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties *pFormatProperties)
{
    unix_funcs->p_vkGetPhysicalDeviceFormatProperties(physicalDevice, format, pFormatProperties);
}

void WINAPI vkGetPhysicalDeviceFormatProperties2(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties2 *pFormatProperties)
{
    unix_funcs->p_vkGetPhysicalDeviceFormatProperties2(physicalDevice, format, pFormatProperties);
}

void WINAPI vkGetPhysicalDeviceFormatProperties2KHR(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties2 *pFormatProperties)
{
    unix_funcs->p_vkGetPhysicalDeviceFormatProperties2KHR(physicalDevice, format, pFormatProperties);
}

VkResult WINAPI vkGetPhysicalDeviceFragmentShadingRatesKHR(VkPhysicalDevice physicalDevice, uint32_t *pFragmentShadingRateCount, VkPhysicalDeviceFragmentShadingRateKHR *pFragmentShadingRates)
{
    return unix_funcs->p_vkGetPhysicalDeviceFragmentShadingRatesKHR(physicalDevice, pFragmentShadingRateCount, pFragmentShadingRates);
}

VkResult WINAPI vkGetPhysicalDeviceImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, VkImageFormatProperties *pImageFormatProperties)
{
    return unix_funcs->p_vkGetPhysicalDeviceImageFormatProperties(physicalDevice, format, type, tiling, usage, flags, pImageFormatProperties);
}

VkResult WINAPI vkGetPhysicalDeviceImageFormatProperties2(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceImageFormatInfo2 *pImageFormatInfo, VkImageFormatProperties2 *pImageFormatProperties)
{
    return unix_funcs->p_vkGetPhysicalDeviceImageFormatProperties2(physicalDevice, pImageFormatInfo, pImageFormatProperties);
}

VkResult WINAPI vkGetPhysicalDeviceImageFormatProperties2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceImageFormatInfo2 *pImageFormatInfo, VkImageFormatProperties2 *pImageFormatProperties)
{
    return unix_funcs->p_vkGetPhysicalDeviceImageFormatProperties2KHR(physicalDevice, pImageFormatInfo, pImageFormatProperties);
}

void WINAPI vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties *pMemoryProperties)
{
    unix_funcs->p_vkGetPhysicalDeviceMemoryProperties(physicalDevice, pMemoryProperties);
}

void WINAPI vkGetPhysicalDeviceMemoryProperties2(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties2 *pMemoryProperties)
{
    unix_funcs->p_vkGetPhysicalDeviceMemoryProperties2(physicalDevice, pMemoryProperties);
}

void WINAPI vkGetPhysicalDeviceMemoryProperties2KHR(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties2 *pMemoryProperties)
{
    unix_funcs->p_vkGetPhysicalDeviceMemoryProperties2KHR(physicalDevice, pMemoryProperties);
}

void WINAPI vkGetPhysicalDeviceMultisamplePropertiesEXT(VkPhysicalDevice physicalDevice, VkSampleCountFlagBits samples, VkMultisamplePropertiesEXT *pMultisampleProperties)
{
    unix_funcs->p_vkGetPhysicalDeviceMultisamplePropertiesEXT(physicalDevice, samples, pMultisampleProperties);
}

VkResult WINAPI vkGetPhysicalDevicePresentRectanglesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t *pRectCount, VkRect2D *pRects)
{
    return unix_funcs->p_vkGetPhysicalDevicePresentRectanglesKHR(physicalDevice, surface, pRectCount, pRects);
}

void WINAPI vkGetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties *pProperties)
{
    unix_funcs->p_vkGetPhysicalDeviceProperties(physicalDevice, pProperties);
}

void WINAPI vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR(VkPhysicalDevice physicalDevice, const VkQueryPoolPerformanceCreateInfoKHR *pPerformanceQueryCreateInfo, uint32_t *pNumPasses)
{
    unix_funcs->p_vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR(physicalDevice, pPerformanceQueryCreateInfo, pNumPasses);
}

void WINAPI vkGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice, uint32_t *pQueueFamilyPropertyCount, VkQueueFamilyProperties *pQueueFamilyProperties)
{
    unix_funcs->p_vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
}

void WINAPI vkGetPhysicalDeviceQueueFamilyProperties2(VkPhysicalDevice physicalDevice, uint32_t *pQueueFamilyPropertyCount, VkQueueFamilyProperties2 *pQueueFamilyProperties)
{
    unix_funcs->p_vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
}

void WINAPI vkGetPhysicalDeviceQueueFamilyProperties2KHR(VkPhysicalDevice physicalDevice, uint32_t *pQueueFamilyPropertyCount, VkQueueFamilyProperties2 *pQueueFamilyProperties)
{
    unix_funcs->p_vkGetPhysicalDeviceQueueFamilyProperties2KHR(physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
}

void WINAPI vkGetPhysicalDeviceSparseImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkSampleCountFlagBits samples, VkImageUsageFlags usage, VkImageTiling tiling, uint32_t *pPropertyCount, VkSparseImageFormatProperties *pProperties)
{
    unix_funcs->p_vkGetPhysicalDeviceSparseImageFormatProperties(physicalDevice, format, type, samples, usage, tiling, pPropertyCount, pProperties);
}

void WINAPI vkGetPhysicalDeviceSparseImageFormatProperties2(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSparseImageFormatInfo2 *pFormatInfo, uint32_t *pPropertyCount, VkSparseImageFormatProperties2 *pProperties)
{
    unix_funcs->p_vkGetPhysicalDeviceSparseImageFormatProperties2(physicalDevice, pFormatInfo, pPropertyCount, pProperties);
}

void WINAPI vkGetPhysicalDeviceSparseImageFormatProperties2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSparseImageFormatInfo2 *pFormatInfo, uint32_t *pPropertyCount, VkSparseImageFormatProperties2 *pProperties)
{
    unix_funcs->p_vkGetPhysicalDeviceSparseImageFormatProperties2KHR(physicalDevice, pFormatInfo, pPropertyCount, pProperties);
}

VkResult WINAPI vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV(VkPhysicalDevice physicalDevice, uint32_t *pCombinationCount, VkFramebufferMixedSamplesCombinationNV *pCombinations)
{
    return unix_funcs->p_vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV(physicalDevice, pCombinationCount, pCombinations);
}

VkResult WINAPI vkGetPhysicalDeviceSurfaceCapabilities2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR *pSurfaceInfo, VkSurfaceCapabilities2KHR *pSurfaceCapabilities)
{
    return unix_funcs->p_vkGetPhysicalDeviceSurfaceCapabilities2KHR(physicalDevice, pSurfaceInfo, pSurfaceCapabilities);
}

VkResult WINAPI vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR *pSurfaceCapabilities)
{
    return unix_funcs->p_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, pSurfaceCapabilities);
}

VkResult WINAPI vkGetPhysicalDeviceSurfaceFormats2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR *pSurfaceInfo, uint32_t *pSurfaceFormatCount, VkSurfaceFormat2KHR *pSurfaceFormats)
{
    return unix_funcs->p_vkGetPhysicalDeviceSurfaceFormats2KHR(physicalDevice, pSurfaceInfo, pSurfaceFormatCount, pSurfaceFormats);
}

VkResult WINAPI vkGetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t *pSurfaceFormatCount, VkSurfaceFormatKHR *pSurfaceFormats)
{
    return unix_funcs->p_vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, pSurfaceFormatCount, pSurfaceFormats);
}

VkResult WINAPI vkGetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t *pPresentModeCount, VkPresentModeKHR *pPresentModes)
{
    return unix_funcs->p_vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, pPresentModeCount, pPresentModes);
}

VkResult WINAPI vkGetPhysicalDeviceSurfaceSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, VkSurfaceKHR surface, VkBool32 *pSupported)
{
    return unix_funcs->p_vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, surface, pSupported);
}

VkResult WINAPI vkGetPhysicalDeviceToolPropertiesEXT(VkPhysicalDevice physicalDevice, uint32_t *pToolCount, VkPhysicalDeviceToolPropertiesEXT *pToolProperties)
{
    return unix_funcs->p_vkGetPhysicalDeviceToolPropertiesEXT(physicalDevice, pToolCount, pToolProperties);
}

VkBool32 WINAPI vkGetPhysicalDeviceWin32PresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex)
{
    return unix_funcs->p_vkGetPhysicalDeviceWin32PresentationSupportKHR(physicalDevice, queueFamilyIndex);
}

VkResult WINAPI vkGetPipelineCacheData(VkDevice device, VkPipelineCache pipelineCache, size_t *pDataSize, void *pData)
{
    return unix_funcs->p_vkGetPipelineCacheData(device, pipelineCache, pDataSize, pData);
}

VkResult WINAPI vkGetPipelineExecutableInternalRepresentationsKHR(VkDevice device, const VkPipelineExecutableInfoKHR *pExecutableInfo, uint32_t *pInternalRepresentationCount, VkPipelineExecutableInternalRepresentationKHR *pInternalRepresentations)
{
    return unix_funcs->p_vkGetPipelineExecutableInternalRepresentationsKHR(device, pExecutableInfo, pInternalRepresentationCount, pInternalRepresentations);
}

VkResult WINAPI vkGetPipelineExecutablePropertiesKHR(VkDevice device, const VkPipelineInfoKHR *pPipelineInfo, uint32_t *pExecutableCount, VkPipelineExecutablePropertiesKHR *pProperties)
{
    return unix_funcs->p_vkGetPipelineExecutablePropertiesKHR(device, pPipelineInfo, pExecutableCount, pProperties);
}

VkResult WINAPI vkGetPipelineExecutableStatisticsKHR(VkDevice device, const VkPipelineExecutableInfoKHR *pExecutableInfo, uint32_t *pStatisticCount, VkPipelineExecutableStatisticKHR *pStatistics)
{
    return unix_funcs->p_vkGetPipelineExecutableStatisticsKHR(device, pExecutableInfo, pStatisticCount, pStatistics);
}

void WINAPI vkGetPrivateDataEXT(VkDevice device, VkObjectType objectType, uint64_t objectHandle, VkPrivateDataSlotEXT privateDataSlot, uint64_t *pData)
{
    unix_funcs->p_vkGetPrivateDataEXT(device, objectType, objectHandle, privateDataSlot, pData);
}

VkResult WINAPI vkGetQueryPoolResults(VkDevice device, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, size_t dataSize, void *pData, VkDeviceSize stride, VkQueryResultFlags flags)
{
    return unix_funcs->p_vkGetQueryPoolResults(device, queryPool, firstQuery, queryCount, dataSize, pData, stride, flags);
}

void WINAPI vkGetQueueCheckpointData2NV(VkQueue queue, uint32_t *pCheckpointDataCount, VkCheckpointData2NV *pCheckpointData)
{
    unix_funcs->p_vkGetQueueCheckpointData2NV(queue, pCheckpointDataCount, pCheckpointData);
}

void WINAPI vkGetQueueCheckpointDataNV(VkQueue queue, uint32_t *pCheckpointDataCount, VkCheckpointDataNV *pCheckpointData)
{
    unix_funcs->p_vkGetQueueCheckpointDataNV(queue, pCheckpointDataCount, pCheckpointData);
}

VkResult WINAPI vkGetRayTracingCaptureReplayShaderGroupHandlesKHR(VkDevice device, VkPipeline pipeline, uint32_t firstGroup, uint32_t groupCount, size_t dataSize, void *pData)
{
    return unix_funcs->p_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR(device, pipeline, firstGroup, groupCount, dataSize, pData);
}

VkResult WINAPI vkGetRayTracingShaderGroupHandlesKHR(VkDevice device, VkPipeline pipeline, uint32_t firstGroup, uint32_t groupCount, size_t dataSize, void *pData)
{
    return unix_funcs->p_vkGetRayTracingShaderGroupHandlesKHR(device, pipeline, firstGroup, groupCount, dataSize, pData);
}

VkResult WINAPI vkGetRayTracingShaderGroupHandlesNV(VkDevice device, VkPipeline pipeline, uint32_t firstGroup, uint32_t groupCount, size_t dataSize, void *pData)
{
    return unix_funcs->p_vkGetRayTracingShaderGroupHandlesNV(device, pipeline, firstGroup, groupCount, dataSize, pData);
}

VkDeviceSize WINAPI vkGetRayTracingShaderGroupStackSizeKHR(VkDevice device, VkPipeline pipeline, uint32_t group, VkShaderGroupShaderKHR groupShader)
{
    return unix_funcs->p_vkGetRayTracingShaderGroupStackSizeKHR(device, pipeline, group, groupShader);
}

void WINAPI vkGetRenderAreaGranularity(VkDevice device, VkRenderPass renderPass, VkExtent2D *pGranularity)
{
    unix_funcs->p_vkGetRenderAreaGranularity(device, renderPass, pGranularity);
}

VkResult WINAPI vkGetSemaphoreCounterValue(VkDevice device, VkSemaphore semaphore, uint64_t *pValue)
{
    return unix_funcs->p_vkGetSemaphoreCounterValue(device, semaphore, pValue);
}

VkResult WINAPI vkGetSemaphoreCounterValueKHR(VkDevice device, VkSemaphore semaphore, uint64_t *pValue)
{
    return unix_funcs->p_vkGetSemaphoreCounterValueKHR(device, semaphore, pValue);
}

VkResult WINAPI vkGetShaderInfoAMD(VkDevice device, VkPipeline pipeline, VkShaderStageFlagBits shaderStage, VkShaderInfoTypeAMD infoType, size_t *pInfoSize, void *pInfo)
{
    return unix_funcs->p_vkGetShaderInfoAMD(device, pipeline, shaderStage, infoType, pInfoSize, pInfo);
}

VkResult WINAPI vkGetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t *pSwapchainImageCount, VkImage *pSwapchainImages)
{
    return unix_funcs->p_vkGetSwapchainImagesKHR(device, swapchain, pSwapchainImageCount, pSwapchainImages);
}

VkResult WINAPI vkGetValidationCacheDataEXT(VkDevice device, VkValidationCacheEXT validationCache, size_t *pDataSize, void *pData)
{
    return unix_funcs->p_vkGetValidationCacheDataEXT(device, validationCache, pDataSize, pData);
}

VkResult WINAPI vkInitializePerformanceApiINTEL(VkDevice device, const VkInitializePerformanceApiInfoINTEL *pInitializeInfo)
{
    return unix_funcs->p_vkInitializePerformanceApiINTEL(device, pInitializeInfo);
}

VkResult WINAPI vkInvalidateMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount, const VkMappedMemoryRange *pMemoryRanges)
{
    return unix_funcs->p_vkInvalidateMappedMemoryRanges(device, memoryRangeCount, pMemoryRanges);
}

VkResult WINAPI vkMapMemory(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void **ppData)
{
    return unix_funcs->p_vkMapMemory(device, memory, offset, size, flags, ppData);
}

VkResult WINAPI vkMergePipelineCaches(VkDevice device, VkPipelineCache dstCache, uint32_t srcCacheCount, const VkPipelineCache *pSrcCaches)
{
    return unix_funcs->p_vkMergePipelineCaches(device, dstCache, srcCacheCount, pSrcCaches);
}

VkResult WINAPI vkMergeValidationCachesEXT(VkDevice device, VkValidationCacheEXT dstCache, uint32_t srcCacheCount, const VkValidationCacheEXT *pSrcCaches)
{
    return unix_funcs->p_vkMergeValidationCachesEXT(device, dstCache, srcCacheCount, pSrcCaches);
}

void WINAPI vkQueueBeginDebugUtilsLabelEXT(VkQueue queue, const VkDebugUtilsLabelEXT *pLabelInfo)
{
    unix_funcs->p_vkQueueBeginDebugUtilsLabelEXT(queue, pLabelInfo);
}

VkResult WINAPI vkQueueBindSparse(VkQueue queue, uint32_t bindInfoCount, const VkBindSparseInfo *pBindInfo, VkFence fence)
{
    return unix_funcs->p_vkQueueBindSparse(queue, bindInfoCount, pBindInfo, fence);
}

void WINAPI vkQueueEndDebugUtilsLabelEXT(VkQueue queue)
{
    unix_funcs->p_vkQueueEndDebugUtilsLabelEXT(queue);
}

void WINAPI vkQueueInsertDebugUtilsLabelEXT(VkQueue queue, const VkDebugUtilsLabelEXT *pLabelInfo)
{
    unix_funcs->p_vkQueueInsertDebugUtilsLabelEXT(queue, pLabelInfo);
}

VkResult WINAPI vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR *pPresentInfo)
{
    return unix_funcs->p_vkQueuePresentKHR(queue, pPresentInfo);
}

VkResult WINAPI vkQueueSetPerformanceConfigurationINTEL(VkQueue queue, VkPerformanceConfigurationINTEL configuration)
{
    return unix_funcs->p_vkQueueSetPerformanceConfigurationINTEL(queue, configuration);
}

VkResult WINAPI vkQueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo *pSubmits, VkFence fence)
{
    return unix_funcs->p_vkQueueSubmit(queue, submitCount, pSubmits, fence);
}

VkResult WINAPI vkQueueSubmit2KHR(VkQueue queue, uint32_t submitCount, const VkSubmitInfo2KHR *pSubmits, VkFence fence)
{
    return unix_funcs->p_vkQueueSubmit2KHR(queue, submitCount, pSubmits, fence);
}

VkResult WINAPI vkQueueWaitIdle(VkQueue queue)
{
    return unix_funcs->p_vkQueueWaitIdle(queue);
}

VkResult WINAPI vkReleasePerformanceConfigurationINTEL(VkDevice device, VkPerformanceConfigurationINTEL configuration)
{
    return unix_funcs->p_vkReleasePerformanceConfigurationINTEL(device, configuration);
}

void WINAPI vkReleaseProfilingLockKHR(VkDevice device)
{
    unix_funcs->p_vkReleaseProfilingLockKHR(device);
}

VkResult WINAPI vkResetCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags)
{
    return unix_funcs->p_vkResetCommandBuffer(commandBuffer, flags);
}

VkResult WINAPI vkResetCommandPool(VkDevice device, VkCommandPool commandPool, VkCommandPoolResetFlags flags)
{
    return unix_funcs->p_vkResetCommandPool(device, commandPool, flags);
}

VkResult WINAPI vkResetDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorPoolResetFlags flags)
{
    return unix_funcs->p_vkResetDescriptorPool(device, descriptorPool, flags);
}

VkResult WINAPI vkResetEvent(VkDevice device, VkEvent event)
{
    return unix_funcs->p_vkResetEvent(device, event);
}

VkResult WINAPI vkResetFences(VkDevice device, uint32_t fenceCount, const VkFence *pFences)
{
    return unix_funcs->p_vkResetFences(device, fenceCount, pFences);
}

void WINAPI vkResetQueryPool(VkDevice device, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount)
{
    unix_funcs->p_vkResetQueryPool(device, queryPool, firstQuery, queryCount);
}

void WINAPI vkResetQueryPoolEXT(VkDevice device, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount)
{
    unix_funcs->p_vkResetQueryPoolEXT(device, queryPool, firstQuery, queryCount);
}

VkResult WINAPI vkSetDebugUtilsObjectNameEXT(VkDevice device, const VkDebugUtilsObjectNameInfoEXT *pNameInfo)
{
    return unix_funcs->p_vkSetDebugUtilsObjectNameEXT(device, pNameInfo);
}

VkResult WINAPI vkSetDebugUtilsObjectTagEXT(VkDevice device, const VkDebugUtilsObjectTagInfoEXT *pTagInfo)
{
    return unix_funcs->p_vkSetDebugUtilsObjectTagEXT(device, pTagInfo);
}

VkResult WINAPI vkSetEvent(VkDevice device, VkEvent event)
{
    return unix_funcs->p_vkSetEvent(device, event);
}

VkResult WINAPI vkSetPrivateDataEXT(VkDevice device, VkObjectType objectType, uint64_t objectHandle, VkPrivateDataSlotEXT privateDataSlot, uint64_t data)
{
    return unix_funcs->p_vkSetPrivateDataEXT(device, objectType, objectHandle, privateDataSlot, data);
}

VkResult WINAPI vkSignalSemaphore(VkDevice device, const VkSemaphoreSignalInfo *pSignalInfo)
{
    return unix_funcs->p_vkSignalSemaphore(device, pSignalInfo);
}

VkResult WINAPI vkSignalSemaphoreKHR(VkDevice device, const VkSemaphoreSignalInfo *pSignalInfo)
{
    return unix_funcs->p_vkSignalSemaphoreKHR(device, pSignalInfo);
}

void WINAPI vkSubmitDebugUtilsMessageEXT(VkInstance instance, VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData)
{
    unix_funcs->p_vkSubmitDebugUtilsMessageEXT(instance, messageSeverity, messageTypes, pCallbackData);
}

void WINAPI vkTrimCommandPool(VkDevice device, VkCommandPool commandPool, VkCommandPoolTrimFlags flags)
{
    unix_funcs->p_vkTrimCommandPool(device, commandPool, flags);
}

void WINAPI vkTrimCommandPoolKHR(VkDevice device, VkCommandPool commandPool, VkCommandPoolTrimFlags flags)
{
    unix_funcs->p_vkTrimCommandPoolKHR(device, commandPool, flags);
}

void WINAPI vkUninitializePerformanceApiINTEL(VkDevice device)
{
    unix_funcs->p_vkUninitializePerformanceApiINTEL(device);
}

void WINAPI vkUnmapMemory(VkDevice device, VkDeviceMemory memory)
{
    unix_funcs->p_vkUnmapMemory(device, memory);
}

void WINAPI vkUpdateDescriptorSetWithTemplate(VkDevice device, VkDescriptorSet descriptorSet, VkDescriptorUpdateTemplate descriptorUpdateTemplate, const void *pData)
{
    unix_funcs->p_vkUpdateDescriptorSetWithTemplate(device, descriptorSet, descriptorUpdateTemplate, pData);
}

void WINAPI vkUpdateDescriptorSetWithTemplateKHR(VkDevice device, VkDescriptorSet descriptorSet, VkDescriptorUpdateTemplate descriptorUpdateTemplate, const void *pData)
{
    unix_funcs->p_vkUpdateDescriptorSetWithTemplateKHR(device, descriptorSet, descriptorUpdateTemplate, pData);
}

void WINAPI vkUpdateDescriptorSets(VkDevice device, uint32_t descriptorWriteCount, const VkWriteDescriptorSet *pDescriptorWrites, uint32_t descriptorCopyCount, const VkCopyDescriptorSet *pDescriptorCopies)
{
    unix_funcs->p_vkUpdateDescriptorSets(device, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies);
}

VkResult WINAPI vkWaitForFences(VkDevice device, uint32_t fenceCount, const VkFence *pFences, VkBool32 waitAll, uint64_t timeout)
{
    return unix_funcs->p_vkWaitForFences(device, fenceCount, pFences, waitAll, timeout);
}

VkResult WINAPI vkWaitSemaphores(VkDevice device, const VkSemaphoreWaitInfo *pWaitInfo, uint64_t timeout)
{
    return unix_funcs->p_vkWaitSemaphores(device, pWaitInfo, timeout);
}

VkResult WINAPI vkWaitSemaphoresKHR(VkDevice device, const VkSemaphoreWaitInfo *pWaitInfo, uint64_t timeout)
{
    return unix_funcs->p_vkWaitSemaphoresKHR(device, pWaitInfo, timeout);
}

VkResult WINAPI vkWriteAccelerationStructuresPropertiesKHR(VkDevice device, uint32_t accelerationStructureCount, const VkAccelerationStructureKHR *pAccelerationStructures, VkQueryType queryType, size_t dataSize, void *pData, size_t stride)
{
    return unix_funcs->p_vkWriteAccelerationStructuresPropertiesKHR(device, accelerationStructureCount, pAccelerationStructures, queryType, dataSize, pData, stride);
}

static const struct vulkan_func vk_device_dispatch_table[] =
{
    {"vkAcquireNextImage2KHR", &vkAcquireNextImage2KHR},
    {"vkAcquireNextImageKHR", &vkAcquireNextImageKHR},
    {"vkAcquirePerformanceConfigurationINTEL", &vkAcquirePerformanceConfigurationINTEL},
    {"vkAcquireProfilingLockKHR", &vkAcquireProfilingLockKHR},
    {"vkAllocateCommandBuffers", &vkAllocateCommandBuffers},
    {"vkAllocateDescriptorSets", &vkAllocateDescriptorSets},
    {"vkAllocateMemory", &vkAllocateMemory},
    {"vkBeginCommandBuffer", &vkBeginCommandBuffer},
    {"vkBindAccelerationStructureMemoryNV", &vkBindAccelerationStructureMemoryNV},
    {"vkBindBufferMemory", &vkBindBufferMemory},
    {"vkBindBufferMemory2", &vkBindBufferMemory2},
    {"vkBindBufferMemory2KHR", &vkBindBufferMemory2KHR},
    {"vkBindImageMemory", &vkBindImageMemory},
    {"vkBindImageMemory2", &vkBindImageMemory2},
    {"vkBindImageMemory2KHR", &vkBindImageMemory2KHR},
    {"vkBuildAccelerationStructuresKHR", &vkBuildAccelerationStructuresKHR},
    {"vkCmdBeginConditionalRenderingEXT", &vkCmdBeginConditionalRenderingEXT},
    {"vkCmdBeginDebugUtilsLabelEXT", &vkCmdBeginDebugUtilsLabelEXT},
    {"vkCmdBeginQuery", &vkCmdBeginQuery},
    {"vkCmdBeginQueryIndexedEXT", &vkCmdBeginQueryIndexedEXT},
    {"vkCmdBeginRenderPass", &vkCmdBeginRenderPass},
    {"vkCmdBeginRenderPass2", &vkCmdBeginRenderPass2},
    {"vkCmdBeginRenderPass2KHR", &vkCmdBeginRenderPass2KHR},
    {"vkCmdBeginTransformFeedbackEXT", &vkCmdBeginTransformFeedbackEXT},
    {"vkCmdBindDescriptorSets", &vkCmdBindDescriptorSets},
    {"vkCmdBindIndexBuffer", &vkCmdBindIndexBuffer},
    {"vkCmdBindPipeline", &vkCmdBindPipeline},
    {"vkCmdBindPipelineShaderGroupNV", &vkCmdBindPipelineShaderGroupNV},
    {"vkCmdBindShadingRateImageNV", &vkCmdBindShadingRateImageNV},
    {"vkCmdBindTransformFeedbackBuffersEXT", &vkCmdBindTransformFeedbackBuffersEXT},
    {"vkCmdBindVertexBuffers", &vkCmdBindVertexBuffers},
    {"vkCmdBindVertexBuffers2EXT", &vkCmdBindVertexBuffers2EXT},
    {"vkCmdBlitImage", &vkCmdBlitImage},
    {"vkCmdBlitImage2KHR", &vkCmdBlitImage2KHR},
    {"vkCmdBuildAccelerationStructureNV", &vkCmdBuildAccelerationStructureNV},
    {"vkCmdBuildAccelerationStructuresIndirectKHR", &vkCmdBuildAccelerationStructuresIndirectKHR},
    {"vkCmdBuildAccelerationStructuresKHR", &vkCmdBuildAccelerationStructuresKHR},
    {"vkCmdClearAttachments", &vkCmdClearAttachments},
    {"vkCmdClearColorImage", &vkCmdClearColorImage},
    {"vkCmdClearDepthStencilImage", &vkCmdClearDepthStencilImage},
    {"vkCmdCopyAccelerationStructureKHR", &vkCmdCopyAccelerationStructureKHR},
    {"vkCmdCopyAccelerationStructureNV", &vkCmdCopyAccelerationStructureNV},
    {"vkCmdCopyAccelerationStructureToMemoryKHR", &vkCmdCopyAccelerationStructureToMemoryKHR},
    {"vkCmdCopyBuffer", &vkCmdCopyBuffer},
    {"vkCmdCopyBuffer2KHR", &vkCmdCopyBuffer2KHR},
    {"vkCmdCopyBufferToImage", &vkCmdCopyBufferToImage},
    {"vkCmdCopyBufferToImage2KHR", &vkCmdCopyBufferToImage2KHR},
    {"vkCmdCopyImage", &vkCmdCopyImage},
    {"vkCmdCopyImage2KHR", &vkCmdCopyImage2KHR},
    {"vkCmdCopyImageToBuffer", &vkCmdCopyImageToBuffer},
    {"vkCmdCopyImageToBuffer2KHR", &vkCmdCopyImageToBuffer2KHR},
    {"vkCmdCopyMemoryToAccelerationStructureKHR", &vkCmdCopyMemoryToAccelerationStructureKHR},
    {"vkCmdCopyQueryPoolResults", &vkCmdCopyQueryPoolResults},
    {"vkCmdCuLaunchKernelNVX", &vkCmdCuLaunchKernelNVX},
    {"vkCmdDebugMarkerBeginEXT", &vkCmdDebugMarkerBeginEXT},
    {"vkCmdDebugMarkerEndEXT", &vkCmdDebugMarkerEndEXT},
    {"vkCmdDebugMarkerInsertEXT", &vkCmdDebugMarkerInsertEXT},
    {"vkCmdDispatch", &vkCmdDispatch},
    {"vkCmdDispatchBase", &vkCmdDispatchBase},
    {"vkCmdDispatchBaseKHR", &vkCmdDispatchBaseKHR},
    {"vkCmdDispatchIndirect", &vkCmdDispatchIndirect},
    {"vkCmdDraw", &vkCmdDraw},
    {"vkCmdDrawIndexed", &vkCmdDrawIndexed},
    {"vkCmdDrawIndexedIndirect", &vkCmdDrawIndexedIndirect},
    {"vkCmdDrawIndexedIndirectCount", &vkCmdDrawIndexedIndirectCount},
    {"vkCmdDrawIndexedIndirectCountAMD", &vkCmdDrawIndexedIndirectCountAMD},
    {"vkCmdDrawIndexedIndirectCountKHR", &vkCmdDrawIndexedIndirectCountKHR},
    {"vkCmdDrawIndirect", &vkCmdDrawIndirect},
    {"vkCmdDrawIndirectByteCountEXT", &vkCmdDrawIndirectByteCountEXT},
    {"vkCmdDrawIndirectCount", &vkCmdDrawIndirectCount},
    {"vkCmdDrawIndirectCountAMD", &vkCmdDrawIndirectCountAMD},
    {"vkCmdDrawIndirectCountKHR", &vkCmdDrawIndirectCountKHR},
    {"vkCmdDrawMeshTasksIndirectCountNV", &vkCmdDrawMeshTasksIndirectCountNV},
    {"vkCmdDrawMeshTasksIndirectNV", &vkCmdDrawMeshTasksIndirectNV},
    {"vkCmdDrawMeshTasksNV", &vkCmdDrawMeshTasksNV},
    {"vkCmdEndConditionalRenderingEXT", &vkCmdEndConditionalRenderingEXT},
    {"vkCmdEndDebugUtilsLabelEXT", &vkCmdEndDebugUtilsLabelEXT},
    {"vkCmdEndQuery", &vkCmdEndQuery},
    {"vkCmdEndQueryIndexedEXT", &vkCmdEndQueryIndexedEXT},
    {"vkCmdEndRenderPass", &vkCmdEndRenderPass},
    {"vkCmdEndRenderPass2", &vkCmdEndRenderPass2},
    {"vkCmdEndRenderPass2KHR", &vkCmdEndRenderPass2KHR},
    {"vkCmdEndTransformFeedbackEXT", &vkCmdEndTransformFeedbackEXT},
    {"vkCmdExecuteCommands", &vkCmdExecuteCommands},
    {"vkCmdExecuteGeneratedCommandsNV", &vkCmdExecuteGeneratedCommandsNV},
    {"vkCmdFillBuffer", &vkCmdFillBuffer},
    {"vkCmdInsertDebugUtilsLabelEXT", &vkCmdInsertDebugUtilsLabelEXT},
    {"vkCmdNextSubpass", &vkCmdNextSubpass},
    {"vkCmdNextSubpass2", &vkCmdNextSubpass2},
    {"vkCmdNextSubpass2KHR", &vkCmdNextSubpass2KHR},
    {"vkCmdPipelineBarrier", &vkCmdPipelineBarrier},
    {"vkCmdPipelineBarrier2KHR", &vkCmdPipelineBarrier2KHR},
    {"vkCmdPreprocessGeneratedCommandsNV", &vkCmdPreprocessGeneratedCommandsNV},
    {"vkCmdPushConstants", &vkCmdPushConstants},
    {"vkCmdPushDescriptorSetKHR", &vkCmdPushDescriptorSetKHR},
    {"vkCmdPushDescriptorSetWithTemplateKHR", &vkCmdPushDescriptorSetWithTemplateKHR},
    {"vkCmdResetEvent", &vkCmdResetEvent},
    {"vkCmdResetEvent2KHR", &vkCmdResetEvent2KHR},
    {"vkCmdResetQueryPool", &vkCmdResetQueryPool},
    {"vkCmdResolveImage", &vkCmdResolveImage},
    {"vkCmdResolveImage2KHR", &vkCmdResolveImage2KHR},
    {"vkCmdSetBlendConstants", &vkCmdSetBlendConstants},
    {"vkCmdSetCheckpointNV", &vkCmdSetCheckpointNV},
    {"vkCmdSetCoarseSampleOrderNV", &vkCmdSetCoarseSampleOrderNV},
    {"vkCmdSetColorWriteEnableEXT", &vkCmdSetColorWriteEnableEXT},
    {"vkCmdSetCullModeEXT", &vkCmdSetCullModeEXT},
    {"vkCmdSetDepthBias", &vkCmdSetDepthBias},
    {"vkCmdSetDepthBiasEnableEXT", &vkCmdSetDepthBiasEnableEXT},
    {"vkCmdSetDepthBounds", &vkCmdSetDepthBounds},
    {"vkCmdSetDepthBoundsTestEnableEXT", &vkCmdSetDepthBoundsTestEnableEXT},
    {"vkCmdSetDepthCompareOpEXT", &vkCmdSetDepthCompareOpEXT},
    {"vkCmdSetDepthTestEnableEXT", &vkCmdSetDepthTestEnableEXT},
    {"vkCmdSetDepthWriteEnableEXT", &vkCmdSetDepthWriteEnableEXT},
    {"vkCmdSetDeviceMask", &vkCmdSetDeviceMask},
    {"vkCmdSetDeviceMaskKHR", &vkCmdSetDeviceMaskKHR},
    {"vkCmdSetDiscardRectangleEXT", &vkCmdSetDiscardRectangleEXT},
    {"vkCmdSetEvent", &vkCmdSetEvent},
    {"vkCmdSetEvent2KHR", &vkCmdSetEvent2KHR},
    {"vkCmdSetExclusiveScissorNV", &vkCmdSetExclusiveScissorNV},
    {"vkCmdSetFragmentShadingRateEnumNV", &vkCmdSetFragmentShadingRateEnumNV},
    {"vkCmdSetFragmentShadingRateKHR", &vkCmdSetFragmentShadingRateKHR},
    {"vkCmdSetFrontFaceEXT", &vkCmdSetFrontFaceEXT},
    {"vkCmdSetLineStippleEXT", &vkCmdSetLineStippleEXT},
    {"vkCmdSetLineWidth", &vkCmdSetLineWidth},
    {"vkCmdSetLogicOpEXT", &vkCmdSetLogicOpEXT},
    {"vkCmdSetPatchControlPointsEXT", &vkCmdSetPatchControlPointsEXT},
    {"vkCmdSetPerformanceMarkerINTEL", &vkCmdSetPerformanceMarkerINTEL},
    {"vkCmdSetPerformanceOverrideINTEL", &vkCmdSetPerformanceOverrideINTEL},
    {"vkCmdSetPerformanceStreamMarkerINTEL", &vkCmdSetPerformanceStreamMarkerINTEL},
    {"vkCmdSetPrimitiveRestartEnableEXT", &vkCmdSetPrimitiveRestartEnableEXT},
    {"vkCmdSetPrimitiveTopologyEXT", &vkCmdSetPrimitiveTopologyEXT},
    {"vkCmdSetRasterizerDiscardEnableEXT", &vkCmdSetRasterizerDiscardEnableEXT},
    {"vkCmdSetRayTracingPipelineStackSizeKHR", &vkCmdSetRayTracingPipelineStackSizeKHR},
    {"vkCmdSetSampleLocationsEXT", &vkCmdSetSampleLocationsEXT},
    {"vkCmdSetScissor", &vkCmdSetScissor},
    {"vkCmdSetScissorWithCountEXT", &vkCmdSetScissorWithCountEXT},
    {"vkCmdSetStencilCompareMask", &vkCmdSetStencilCompareMask},
    {"vkCmdSetStencilOpEXT", &vkCmdSetStencilOpEXT},
    {"vkCmdSetStencilReference", &vkCmdSetStencilReference},
    {"vkCmdSetStencilTestEnableEXT", &vkCmdSetStencilTestEnableEXT},
    {"vkCmdSetStencilWriteMask", &vkCmdSetStencilWriteMask},
    {"vkCmdSetVertexInputEXT", &vkCmdSetVertexInputEXT},
    {"vkCmdSetViewport", &vkCmdSetViewport},
    {"vkCmdSetViewportShadingRatePaletteNV", &vkCmdSetViewportShadingRatePaletteNV},
    {"vkCmdSetViewportWScalingNV", &vkCmdSetViewportWScalingNV},
    {"vkCmdSetViewportWithCountEXT", &vkCmdSetViewportWithCountEXT},
    {"vkCmdTraceRaysIndirectKHR", &vkCmdTraceRaysIndirectKHR},
    {"vkCmdTraceRaysKHR", &vkCmdTraceRaysKHR},
    {"vkCmdTraceRaysNV", &vkCmdTraceRaysNV},
    {"vkCmdUpdateBuffer", &vkCmdUpdateBuffer},
    {"vkCmdWaitEvents", &vkCmdWaitEvents},
    {"vkCmdWaitEvents2KHR", &vkCmdWaitEvents2KHR},
    {"vkCmdWriteAccelerationStructuresPropertiesKHR", &vkCmdWriteAccelerationStructuresPropertiesKHR},
    {"vkCmdWriteAccelerationStructuresPropertiesNV", &vkCmdWriteAccelerationStructuresPropertiesNV},
    {"vkCmdWriteBufferMarker2AMD", &vkCmdWriteBufferMarker2AMD},
    {"vkCmdWriteBufferMarkerAMD", &vkCmdWriteBufferMarkerAMD},
    {"vkCmdWriteTimestamp", &vkCmdWriteTimestamp},
    {"vkCmdWriteTimestamp2KHR", &vkCmdWriteTimestamp2KHR},
    {"vkCompileDeferredNV", &vkCompileDeferredNV},
    {"vkCopyAccelerationStructureKHR", &vkCopyAccelerationStructureKHR},
    {"vkCopyAccelerationStructureToMemoryKHR", &vkCopyAccelerationStructureToMemoryKHR},
    {"vkCopyMemoryToAccelerationStructureKHR", &vkCopyMemoryToAccelerationStructureKHR},
    {"vkCreateAccelerationStructureKHR", &vkCreateAccelerationStructureKHR},
    {"vkCreateAccelerationStructureNV", &vkCreateAccelerationStructureNV},
    {"vkCreateBuffer", &vkCreateBuffer},
    {"vkCreateBufferView", &vkCreateBufferView},
    {"vkCreateCommandPool", &vkCreateCommandPool},
    {"vkCreateComputePipelines", &vkCreateComputePipelines},
    {"vkCreateCuFunctionNVX", &vkCreateCuFunctionNVX},
    {"vkCreateCuModuleNVX", &vkCreateCuModuleNVX},
    {"vkCreateDeferredOperationKHR", &vkCreateDeferredOperationKHR},
    {"vkCreateDescriptorPool", &vkCreateDescriptorPool},
    {"vkCreateDescriptorSetLayout", &vkCreateDescriptorSetLayout},
    {"vkCreateDescriptorUpdateTemplate", &vkCreateDescriptorUpdateTemplate},
    {"vkCreateDescriptorUpdateTemplateKHR", &vkCreateDescriptorUpdateTemplateKHR},
    {"vkCreateEvent", &vkCreateEvent},
    {"vkCreateFence", &vkCreateFence},
    {"vkCreateFramebuffer", &vkCreateFramebuffer},
    {"vkCreateGraphicsPipelines", &vkCreateGraphicsPipelines},
    {"vkCreateImage", &vkCreateImage},
    {"vkCreateImageView", &vkCreateImageView},
    {"vkCreateIndirectCommandsLayoutNV", &vkCreateIndirectCommandsLayoutNV},
    {"vkCreatePipelineCache", &vkCreatePipelineCache},
    {"vkCreatePipelineLayout", &vkCreatePipelineLayout},
    {"vkCreatePrivateDataSlotEXT", &vkCreatePrivateDataSlotEXT},
    {"vkCreateQueryPool", &vkCreateQueryPool},
    {"vkCreateRayTracingPipelinesKHR", &vkCreateRayTracingPipelinesKHR},
    {"vkCreateRayTracingPipelinesNV", &vkCreateRayTracingPipelinesNV},
    {"vkCreateRenderPass", &vkCreateRenderPass},
    {"vkCreateRenderPass2", &vkCreateRenderPass2},
    {"vkCreateRenderPass2KHR", &vkCreateRenderPass2KHR},
    {"vkCreateSampler", &vkCreateSampler},
    {"vkCreateSamplerYcbcrConversion", &vkCreateSamplerYcbcrConversion},
    {"vkCreateSamplerYcbcrConversionKHR", &vkCreateSamplerYcbcrConversionKHR},
    {"vkCreateSemaphore", &vkCreateSemaphore},
    {"vkCreateShaderModule", &vkCreateShaderModule},
    {"vkCreateSwapchainKHR", &vkCreateSwapchainKHR},
    {"vkCreateValidationCacheEXT", &vkCreateValidationCacheEXT},
    {"vkDebugMarkerSetObjectNameEXT", &vkDebugMarkerSetObjectNameEXT},
    {"vkDebugMarkerSetObjectTagEXT", &vkDebugMarkerSetObjectTagEXT},
    {"vkDeferredOperationJoinKHR", &vkDeferredOperationJoinKHR},
    {"vkDestroyAccelerationStructureKHR", &vkDestroyAccelerationStructureKHR},
    {"vkDestroyAccelerationStructureNV", &vkDestroyAccelerationStructureNV},
    {"vkDestroyBuffer", &vkDestroyBuffer},
    {"vkDestroyBufferView", &vkDestroyBufferView},
    {"vkDestroyCommandPool", &vkDestroyCommandPool},
    {"vkDestroyCuFunctionNVX", &vkDestroyCuFunctionNVX},
    {"vkDestroyCuModuleNVX", &vkDestroyCuModuleNVX},
    {"vkDestroyDeferredOperationKHR", &vkDestroyDeferredOperationKHR},
    {"vkDestroyDescriptorPool", &vkDestroyDescriptorPool},
    {"vkDestroyDescriptorSetLayout", &vkDestroyDescriptorSetLayout},
    {"vkDestroyDescriptorUpdateTemplate", &vkDestroyDescriptorUpdateTemplate},
    {"vkDestroyDescriptorUpdateTemplateKHR", &vkDestroyDescriptorUpdateTemplateKHR},
    {"vkDestroyDevice", &vkDestroyDevice},
    {"vkDestroyEvent", &vkDestroyEvent},
    {"vkDestroyFence", &vkDestroyFence},
    {"vkDestroyFramebuffer", &vkDestroyFramebuffer},
    {"vkDestroyImage", &vkDestroyImage},
    {"vkDestroyImageView", &vkDestroyImageView},
    {"vkDestroyIndirectCommandsLayoutNV", &vkDestroyIndirectCommandsLayoutNV},
    {"vkDestroyPipeline", &vkDestroyPipeline},
    {"vkDestroyPipelineCache", &vkDestroyPipelineCache},
    {"vkDestroyPipelineLayout", &vkDestroyPipelineLayout},
    {"vkDestroyPrivateDataSlotEXT", &vkDestroyPrivateDataSlotEXT},
    {"vkDestroyQueryPool", &vkDestroyQueryPool},
    {"vkDestroyRenderPass", &vkDestroyRenderPass},
    {"vkDestroySampler", &vkDestroySampler},
    {"vkDestroySamplerYcbcrConversion", &vkDestroySamplerYcbcrConversion},
    {"vkDestroySamplerYcbcrConversionKHR", &vkDestroySamplerYcbcrConversionKHR},
    {"vkDestroySemaphore", &vkDestroySemaphore},
    {"vkDestroyShaderModule", &vkDestroyShaderModule},
    {"vkDestroySwapchainKHR", &vkDestroySwapchainKHR},
    {"vkDestroyValidationCacheEXT", &vkDestroyValidationCacheEXT},
    {"vkDeviceWaitIdle", &vkDeviceWaitIdle},
    {"vkEndCommandBuffer", &vkEndCommandBuffer},
    {"vkFlushMappedMemoryRanges", &vkFlushMappedMemoryRanges},
    {"vkFreeCommandBuffers", &vkFreeCommandBuffers},
    {"vkFreeDescriptorSets", &vkFreeDescriptorSets},
    {"vkFreeMemory", &vkFreeMemory},
    {"vkGetAccelerationStructureBuildSizesKHR", &vkGetAccelerationStructureBuildSizesKHR},
    {"vkGetAccelerationStructureDeviceAddressKHR", &vkGetAccelerationStructureDeviceAddressKHR},
    {"vkGetAccelerationStructureHandleNV", &vkGetAccelerationStructureHandleNV},
    {"vkGetAccelerationStructureMemoryRequirementsNV", &vkGetAccelerationStructureMemoryRequirementsNV},
    {"vkGetBufferDeviceAddress", &vkGetBufferDeviceAddress},
    {"vkGetBufferDeviceAddressEXT", &vkGetBufferDeviceAddressEXT},
    {"vkGetBufferDeviceAddressKHR", &vkGetBufferDeviceAddressKHR},
    {"vkGetBufferMemoryRequirements", &vkGetBufferMemoryRequirements},
    {"vkGetBufferMemoryRequirements2", &vkGetBufferMemoryRequirements2},
    {"vkGetBufferMemoryRequirements2KHR", &vkGetBufferMemoryRequirements2KHR},
    {"vkGetBufferOpaqueCaptureAddress", &vkGetBufferOpaqueCaptureAddress},
    {"vkGetBufferOpaqueCaptureAddressKHR", &vkGetBufferOpaqueCaptureAddressKHR},
    {"vkGetCalibratedTimestampsEXT", &vkGetCalibratedTimestampsEXT},
    {"vkGetDeferredOperationMaxConcurrencyKHR", &vkGetDeferredOperationMaxConcurrencyKHR},
    {"vkGetDeferredOperationResultKHR", &vkGetDeferredOperationResultKHR},
    {"vkGetDescriptorSetLayoutSupport", &vkGetDescriptorSetLayoutSupport},
    {"vkGetDescriptorSetLayoutSupportKHR", &vkGetDescriptorSetLayoutSupportKHR},
    {"vkGetDeviceAccelerationStructureCompatibilityKHR", &vkGetDeviceAccelerationStructureCompatibilityKHR},
    {"vkGetDeviceGroupPeerMemoryFeatures", &vkGetDeviceGroupPeerMemoryFeatures},
    {"vkGetDeviceGroupPeerMemoryFeaturesKHR", &vkGetDeviceGroupPeerMemoryFeaturesKHR},
    {"vkGetDeviceGroupPresentCapabilitiesKHR", &vkGetDeviceGroupPresentCapabilitiesKHR},
    {"vkGetDeviceGroupSurfacePresentModesKHR", &vkGetDeviceGroupSurfacePresentModesKHR},
    {"vkGetDeviceMemoryCommitment", &vkGetDeviceMemoryCommitment},
    {"vkGetDeviceMemoryOpaqueCaptureAddress", &vkGetDeviceMemoryOpaqueCaptureAddress},
    {"vkGetDeviceMemoryOpaqueCaptureAddressKHR", &vkGetDeviceMemoryOpaqueCaptureAddressKHR},
    {"vkGetDeviceProcAddr", &vkGetDeviceProcAddr},
    {"vkGetDeviceQueue", &vkGetDeviceQueue},
    {"vkGetDeviceQueue2", &vkGetDeviceQueue2},
    {"vkGetEventStatus", &vkGetEventStatus},
    {"vkGetFenceStatus", &vkGetFenceStatus},
    {"vkGetGeneratedCommandsMemoryRequirementsNV", &vkGetGeneratedCommandsMemoryRequirementsNV},
    {"vkGetImageMemoryRequirements", &vkGetImageMemoryRequirements},
    {"vkGetImageMemoryRequirements2", &vkGetImageMemoryRequirements2},
    {"vkGetImageMemoryRequirements2KHR", &vkGetImageMemoryRequirements2KHR},
    {"vkGetImageSparseMemoryRequirements", &vkGetImageSparseMemoryRequirements},
    {"vkGetImageSparseMemoryRequirements2", &vkGetImageSparseMemoryRequirements2},
    {"vkGetImageSparseMemoryRequirements2KHR", &vkGetImageSparseMemoryRequirements2KHR},
    {"vkGetImageSubresourceLayout", &vkGetImageSubresourceLayout},
    {"vkGetImageViewAddressNVX", &vkGetImageViewAddressNVX},
    {"vkGetImageViewHandleNVX", &vkGetImageViewHandleNVX},
    {"vkGetMemoryHostPointerPropertiesEXT", &vkGetMemoryHostPointerPropertiesEXT},
    {"vkGetPerformanceParameterINTEL", &vkGetPerformanceParameterINTEL},
    {"vkGetPipelineCacheData", &vkGetPipelineCacheData},
    {"vkGetPipelineExecutableInternalRepresentationsKHR", &vkGetPipelineExecutableInternalRepresentationsKHR},
    {"vkGetPipelineExecutablePropertiesKHR", &vkGetPipelineExecutablePropertiesKHR},
    {"vkGetPipelineExecutableStatisticsKHR", &vkGetPipelineExecutableStatisticsKHR},
    {"vkGetPrivateDataEXT", &vkGetPrivateDataEXT},
    {"vkGetQueryPoolResults", &vkGetQueryPoolResults},
    {"vkGetQueueCheckpointData2NV", &vkGetQueueCheckpointData2NV},
    {"vkGetQueueCheckpointDataNV", &vkGetQueueCheckpointDataNV},
    {"vkGetRayTracingCaptureReplayShaderGroupHandlesKHR", &vkGetRayTracingCaptureReplayShaderGroupHandlesKHR},
    {"vkGetRayTracingShaderGroupHandlesKHR", &vkGetRayTracingShaderGroupHandlesKHR},
    {"vkGetRayTracingShaderGroupHandlesNV", &vkGetRayTracingShaderGroupHandlesNV},
    {"vkGetRayTracingShaderGroupStackSizeKHR", &vkGetRayTracingShaderGroupStackSizeKHR},
    {"vkGetRenderAreaGranularity", &vkGetRenderAreaGranularity},
    {"vkGetSemaphoreCounterValue", &vkGetSemaphoreCounterValue},
    {"vkGetSemaphoreCounterValueKHR", &vkGetSemaphoreCounterValueKHR},
    {"vkGetShaderInfoAMD", &vkGetShaderInfoAMD},
    {"vkGetSwapchainImagesKHR", &vkGetSwapchainImagesKHR},
    {"vkGetValidationCacheDataEXT", &vkGetValidationCacheDataEXT},
    {"vkInitializePerformanceApiINTEL", &vkInitializePerformanceApiINTEL},
    {"vkInvalidateMappedMemoryRanges", &vkInvalidateMappedMemoryRanges},
    {"vkMapMemory", &vkMapMemory},
    {"vkMergePipelineCaches", &vkMergePipelineCaches},
    {"vkMergeValidationCachesEXT", &vkMergeValidationCachesEXT},
    {"vkQueueBeginDebugUtilsLabelEXT", &vkQueueBeginDebugUtilsLabelEXT},
    {"vkQueueBindSparse", &vkQueueBindSparse},
    {"vkQueueEndDebugUtilsLabelEXT", &vkQueueEndDebugUtilsLabelEXT},
    {"vkQueueInsertDebugUtilsLabelEXT", &vkQueueInsertDebugUtilsLabelEXT},
    {"vkQueuePresentKHR", &vkQueuePresentKHR},
    {"vkQueueSetPerformanceConfigurationINTEL", &vkQueueSetPerformanceConfigurationINTEL},
    {"vkQueueSubmit", &vkQueueSubmit},
    {"vkQueueSubmit2KHR", &vkQueueSubmit2KHR},
    {"vkQueueWaitIdle", &vkQueueWaitIdle},
    {"vkReleasePerformanceConfigurationINTEL", &vkReleasePerformanceConfigurationINTEL},
    {"vkReleaseProfilingLockKHR", &vkReleaseProfilingLockKHR},
    {"vkResetCommandBuffer", &vkResetCommandBuffer},
    {"vkResetCommandPool", &vkResetCommandPool},
    {"vkResetDescriptorPool", &vkResetDescriptorPool},
    {"vkResetEvent", &vkResetEvent},
    {"vkResetFences", &vkResetFences},
    {"vkResetQueryPool", &vkResetQueryPool},
    {"vkResetQueryPoolEXT", &vkResetQueryPoolEXT},
    {"vkSetDebugUtilsObjectNameEXT", &vkSetDebugUtilsObjectNameEXT},
    {"vkSetDebugUtilsObjectTagEXT", &vkSetDebugUtilsObjectTagEXT},
    {"vkSetEvent", &vkSetEvent},
    {"vkSetPrivateDataEXT", &vkSetPrivateDataEXT},
    {"vkSignalSemaphore", &vkSignalSemaphore},
    {"vkSignalSemaphoreKHR", &vkSignalSemaphoreKHR},
    {"vkTrimCommandPool", &vkTrimCommandPool},
    {"vkTrimCommandPoolKHR", &vkTrimCommandPoolKHR},
    {"vkUninitializePerformanceApiINTEL", &vkUninitializePerformanceApiINTEL},
    {"vkUnmapMemory", &vkUnmapMemory},
    {"vkUpdateDescriptorSetWithTemplate", &vkUpdateDescriptorSetWithTemplate},
    {"vkUpdateDescriptorSetWithTemplateKHR", &vkUpdateDescriptorSetWithTemplateKHR},
    {"vkUpdateDescriptorSets", &vkUpdateDescriptorSets},
    {"vkWaitForFences", &vkWaitForFences},
    {"vkWaitSemaphores", &vkWaitSemaphores},
    {"vkWaitSemaphoresKHR", &vkWaitSemaphoresKHR},
    {"vkWriteAccelerationStructuresPropertiesKHR", &vkWriteAccelerationStructuresPropertiesKHR},
};

static const struct vulkan_func vk_phys_dev_dispatch_table[] =
{
    {"vkCreateDevice", &vkCreateDevice},
    {"vkEnumerateDeviceExtensionProperties", &vkEnumerateDeviceExtensionProperties},
    {"vkEnumerateDeviceLayerProperties", &vkEnumerateDeviceLayerProperties},
    {"vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR", &vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR},
    {"vkGetPhysicalDeviceCalibrateableTimeDomainsEXT", &vkGetPhysicalDeviceCalibrateableTimeDomainsEXT},
    {"vkGetPhysicalDeviceCooperativeMatrixPropertiesNV", &vkGetPhysicalDeviceCooperativeMatrixPropertiesNV},
    {"vkGetPhysicalDeviceExternalBufferProperties", &vkGetPhysicalDeviceExternalBufferProperties},
    {"vkGetPhysicalDeviceExternalBufferPropertiesKHR", &vkGetPhysicalDeviceExternalBufferPropertiesKHR},
    {"vkGetPhysicalDeviceExternalFenceProperties", &vkGetPhysicalDeviceExternalFenceProperties},
    {"vkGetPhysicalDeviceExternalFencePropertiesKHR", &vkGetPhysicalDeviceExternalFencePropertiesKHR},
    {"vkGetPhysicalDeviceExternalSemaphoreProperties", &vkGetPhysicalDeviceExternalSemaphoreProperties},
    {"vkGetPhysicalDeviceExternalSemaphorePropertiesKHR", &vkGetPhysicalDeviceExternalSemaphorePropertiesKHR},
    {"vkGetPhysicalDeviceFeatures", &vkGetPhysicalDeviceFeatures},
    {"vkGetPhysicalDeviceFeatures2", &vkGetPhysicalDeviceFeatures2},
    {"vkGetPhysicalDeviceFeatures2KHR", &vkGetPhysicalDeviceFeatures2KHR},
    {"vkGetPhysicalDeviceFormatProperties", &vkGetPhysicalDeviceFormatProperties},
    {"vkGetPhysicalDeviceFormatProperties2", &vkGetPhysicalDeviceFormatProperties2},
    {"vkGetPhysicalDeviceFormatProperties2KHR", &vkGetPhysicalDeviceFormatProperties2KHR},
    {"vkGetPhysicalDeviceFragmentShadingRatesKHR", &vkGetPhysicalDeviceFragmentShadingRatesKHR},
    {"vkGetPhysicalDeviceImageFormatProperties", &vkGetPhysicalDeviceImageFormatProperties},
    {"vkGetPhysicalDeviceImageFormatProperties2", &vkGetPhysicalDeviceImageFormatProperties2},
    {"vkGetPhysicalDeviceImageFormatProperties2KHR", &vkGetPhysicalDeviceImageFormatProperties2KHR},
    {"vkGetPhysicalDeviceMemoryProperties", &vkGetPhysicalDeviceMemoryProperties},
    {"vkGetPhysicalDeviceMemoryProperties2", &vkGetPhysicalDeviceMemoryProperties2},
    {"vkGetPhysicalDeviceMemoryProperties2KHR", &vkGetPhysicalDeviceMemoryProperties2KHR},
    {"vkGetPhysicalDeviceMultisamplePropertiesEXT", &vkGetPhysicalDeviceMultisamplePropertiesEXT},
    {"vkGetPhysicalDevicePresentRectanglesKHR", &vkGetPhysicalDevicePresentRectanglesKHR},
    {"vkGetPhysicalDeviceProperties", &vkGetPhysicalDeviceProperties},
    {"vkGetPhysicalDeviceProperties2", &vkGetPhysicalDeviceProperties2},
    {"vkGetPhysicalDeviceProperties2KHR", &vkGetPhysicalDeviceProperties2KHR},
    {"vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR", &vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR},
    {"vkGetPhysicalDeviceQueueFamilyProperties", &vkGetPhysicalDeviceQueueFamilyProperties},
    {"vkGetPhysicalDeviceQueueFamilyProperties2", &vkGetPhysicalDeviceQueueFamilyProperties2},
    {"vkGetPhysicalDeviceQueueFamilyProperties2KHR", &vkGetPhysicalDeviceQueueFamilyProperties2KHR},
    {"vkGetPhysicalDeviceSparseImageFormatProperties", &vkGetPhysicalDeviceSparseImageFormatProperties},
    {"vkGetPhysicalDeviceSparseImageFormatProperties2", &vkGetPhysicalDeviceSparseImageFormatProperties2},
    {"vkGetPhysicalDeviceSparseImageFormatProperties2KHR", &vkGetPhysicalDeviceSparseImageFormatProperties2KHR},
    {"vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV", &vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV},
    {"vkGetPhysicalDeviceSurfaceCapabilities2KHR", &vkGetPhysicalDeviceSurfaceCapabilities2KHR},
    {"vkGetPhysicalDeviceSurfaceCapabilitiesKHR", &vkGetPhysicalDeviceSurfaceCapabilitiesKHR},
    {"vkGetPhysicalDeviceSurfaceFormats2KHR", &vkGetPhysicalDeviceSurfaceFormats2KHR},
    {"vkGetPhysicalDeviceSurfaceFormatsKHR", &vkGetPhysicalDeviceSurfaceFormatsKHR},
    {"vkGetPhysicalDeviceSurfacePresentModesKHR", &vkGetPhysicalDeviceSurfacePresentModesKHR},
    {"vkGetPhysicalDeviceSurfaceSupportKHR", &vkGetPhysicalDeviceSurfaceSupportKHR},
    {"vkGetPhysicalDeviceToolPropertiesEXT", &vkGetPhysicalDeviceToolPropertiesEXT},
    {"vkGetPhysicalDeviceWin32PresentationSupportKHR", &vkGetPhysicalDeviceWin32PresentationSupportKHR},
};

static const struct vulkan_func vk_instance_dispatch_table[] =
{
    {"vkCreateDebugReportCallbackEXT", &vkCreateDebugReportCallbackEXT},
    {"vkCreateDebugUtilsMessengerEXT", &vkCreateDebugUtilsMessengerEXT},
    {"vkCreateWin32SurfaceKHR", &vkCreateWin32SurfaceKHR},
    {"vkDebugReportMessageEXT", &vkDebugReportMessageEXT},
    {"vkDestroyDebugReportCallbackEXT", &vkDestroyDebugReportCallbackEXT},
    {"vkDestroyDebugUtilsMessengerEXT", &vkDestroyDebugUtilsMessengerEXT},
    {"vkDestroyInstance", &vkDestroyInstance},
    {"vkDestroySurfaceKHR", &vkDestroySurfaceKHR},
    {"vkEnumeratePhysicalDeviceGroups", &vkEnumeratePhysicalDeviceGroups},
    {"vkEnumeratePhysicalDeviceGroupsKHR", &vkEnumeratePhysicalDeviceGroupsKHR},
    {"vkEnumeratePhysicalDevices", &vkEnumeratePhysicalDevices},
    {"vkSubmitDebugUtilsMessageEXT", &vkSubmitDebugUtilsMessageEXT},
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
